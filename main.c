#include "conf.c"
#include "monitor.c"
#include "tree.c"

#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#define buffSize 255
#define commandLength 255
#define iconPath "$PWD/Resources/icon.png"

bool highRamUsage = false; // Флаг, указывающий, что использование оперативной памяти высоко
bool highSwapUsage = false; // Флаг, указывающий, что использование оперативной памяти высоко
bool* highDirectoryUsage = NULL; // Флаг, указывающий, что использование оперативной памяти высоко

TreeNode *alarmTree = NULL;
int alarmId = 0;

settings sett;

int msgCount = 0;
pid_t ppid;
pid_t cpid;
int _pipeCommands[2], _pipeRespond[2];

void USR1Listener(int signal) {
    if (signal != SIGUSR1) return;
    msgCount++;
}

void INTListener(int signal) {
    if (signal != SIGINT) return;
    exitSetupServer();
    exit(0);
}

int sockfd, connfd, len;
struct sockaddr_in servaddr, cli;

void alarmWindow(const char* message) {
    char com[commandLength] = {0};
    sprintf(com, "notify-send -u normal -i %s Memoger \"%s\"", iconPath, message);
    system(com);
}

// Функция для проверки использования оперативной памяти
int checkRamUsage() {
    long ram_usage = get_ram_usage();
    if (ram_usage > sett.limitRam && !highRamUsage) {
        highRamUsage = true;
        syslog(LOG_WARNING, "RAM Usage is over the limit");
        alarmTree = addAlarmToTree("RAM Usage is over the limit", alarmId++, alarmTree);

        alarmWindow("RAM Usage is over the limit");
        return 0;
    } else if (ram_usage <= (sett.limitRam * sett.hyster_ram) && highRamUsage) {
        highRamUsage = false;
        syslog(LOG_NOTICE, "RAM Usage is back to normal state");
        alarmTree = addAlarmToTree("RAM Usage is back to normal state", alarmId++, alarmTree);
        return 1;
    }
}

// Функция для проверки использования Swap памяти
int checkSwapUsage() {
    long swap_usage = get_swap_usage();
    if (swap_usage > sett.limitSwap && !highSwapUsage) {
        highSwapUsage = true;
        syslog(LOG_WARNING, "SWAP Usage is over the limit");
        alarmTree = addAlarmToTree("SWAP Usage is over the limit", alarmId++, alarmTree);

        alarmWindow("SWAP Usage is over the limit");
        return 0;
    } else if (swap_usage <= (sett.limitSwap * sett.hyster_swap) && highSwapUsage) {
        highSwapUsage = false;
        syslog(LOG_NOTICE, "SWAP Usage is back to normal state");
        alarmTree = addAlarmToTree("SWAP Usage is back to normal state", alarmId++, alarmTree);
        return 1;
    }
}

// Функция для проверки использования директории
int checkDirectoryUsage(traceddir dir, bool *isOverLimit) {
    long directorySize = directory_size(dir.path);
    if ((directorySize > dir.critical_size) && !(*isOverLimit)) {
        (*isOverLimit) = true;
        syslog(LOG_WARNING, "Size of dir %s is over the personal limit", dir.path);
        char buff[buffSize] = {0};
        sprintf(buff, "Size of dir %s is over the personal limit", dir.path);
        alarmTree = addAlarmToTree(buff, alarmId++, alarmTree);

        alarmWindow(buff);
        return 0;
    } else if ((directorySize <= (dir.critical_size * sett.hyster_dir)) && (*isOverLimit)) {
        (*isOverLimit) = false;
        syslog(LOG_NOTICE, "Size of dir %s is back to normal", dir.path);
        char buff[buffSize] = {0};
        sprintf(buff, "SSize of dir %s is back to normal", dir.path);
        alarmTree = addAlarmToTree(buff, alarmId++, alarmTree);
        return 1;
    }
    return -1;
}

//Для первичной проверки и настройки необходимого функционала
int startUpSetupServer() {
    daemon(1,1);
    signal(SIGINT, INTListener);
    openlog("memoger", LOG_PID, LOG_USER);
    sett = parseSettings();
    highDirectoryUsage=(bool*)malloc(sizeof(bool)*sett.countDirs);
    for (int i = 0; i < sett.countDirs; i++) {
        highDirectoryUsage[i]=false;
	}
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_CRIT, "Server socket creation failed");
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(sett.port);
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        syslog(LOG_CRIT, "Server socket bind failed:%s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ((listen(sockfd, 5)) != 0) {
        syslog(LOG_CRIT, "Listen failed");
        exit(0);
    }
    len = sizeof(cli);
    pipe(_pipeCommands);
    pipe(_pipeRespond);
    ppid = getpid();
    cpid = fork();
    if (cpid == 0) {
        serverAccepter();
    } else if (cpid < 0) {
        // Обработка ошибки
        syslog(LOG_CRIT, "Fork failed");
        exit(EXIT_FAILURE);
    } else {
        signal(SIGUSR1, USR1Listener);
    }
    syslog(LOG_NOTICE, "Server is running");
    return 0;
}

void serverAccepter() {
    while (1) {
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
		if (connfd < 0) {
            syslog(LOG_CRIT, "Server accept failed");
            kill(ppid, SIGTERM);
            exit(0);
        }
        syslog(LOG_INFO, "New connection to server");
        kill(ppid, SIGUSR1); //говорим главному, что есть клиент
        char buff[buffSize] = {0};

        read(connfd, buff, sizeof(buff));
        write(_pipeCommands[1], buff, strlen(buff)); //передаем команду

        while (1) { //передача информации клиенту
            memset(buff, 0, sizeof(buff));
            read(_pipeRespond[0], buff, sizeof(buff));
            int len = strlen(buff);
            write(connfd, buff, len);
            if (buff[len - 1] == '\r') {
				break;
			}
        }
        syslog(LOG_INFO, "Respond end");
        sleep(1);
        close(connfd);
    }
}

//Для успешного окончания работы, закрытия всех открытых файлов, сохранения информации
int exitSetupServer() {
    free(highDirectoryUsage);
    kill(SIGINT, cpid);
    closelog();
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    freeAlarmTree(alarmTree); //Освобождение памяти
    syslog(LOG_NOTICE, "Server shuting down");
    return 0;
}

int mainServer() {
	readFile();
    startUpSetupServer();
    int flag = 0;
    int exitFlag = 0;
    while (1) {
        for (int secs = 0; secs < sett.period; secs++){
            if (msgCount > 0) {
                char buff[buffSize] = {0};
                read(_pipeCommands[0], buff, sizeof(buff)); //чтение команды
                char* token = strtok(buff,"|");
                switch (atoi(token)) { //дешифрока/свич-кейс
                    case 0: { //show
                        token=strtok(NULL,"|");
                        switch(atoi(token)) {
                            case 0: { //node by id
                                int targetId = atoi(strtok(NULL, "|")); // Get the target alarm ID
                                char response[buffSize];
                                response[0] = '\0';

                                TreeNode* currentAlarm = alarmTree;
                                while (currentAlarm != NULL) {
									if (currentAlarm->id == targetId) {
										sprintf(response, "Alarm ID: %d\nTimestamp: %s\nMessage: %s\n",
											currentAlarm->id, currentAlarm->timestamp, currentAlarm->message);
										break;
									}
									currentAlarm = currentAlarm->right;
                                }
                                write(_pipeRespond[1], response, strlen(response));
                                break;
                            }
                            case 1: { //ram
                                long ram_usage = get_ram_usage();
                                memset(buff,0,sizeof(buff));
                                sprintf(buff, "RAM Usage: %ld/%ld\n", ram_usage,get_ram_total());
                                write(_pipeRespond[1], buff, strlen(buff));
                                break;
                            }
                            case 2: { //swap
                                long swap_usage = get_swap_usage();
                                sprintf(buff, "Swap Usage: %ld\n", swap_usage);
                                write(_pipeRespond[1], buff, strlen(buff));
                                break;
                            }
                            case 3: { //dir
                                char response[buffSize];
                                response[0] = '\0';

                                for (int i = 0; i < sett.countDirs; i++) {
									traceddir dir = sett.dirs[i];
									long directorySize = directory_size(dir.path);
									sprintf(response, "%sDirectory: %s, Size: %ld\n", response, dir.path, directorySize);
                                }

                                write(_pipeRespond[1], response, strlen(response));

                                break;
                            }
                            case 6: { //full tree
                                showTree(alarmTree, _pipeRespond[1],buffSize);
                                break;
                            }
                            case 7: { //show all
                                //RAM
                                long ram_usage = get_ram_usage();
                                memset(buff, 0, sizeof(buff));
                                sprintf(buff, "RAM Usage: %ld/%ld\n", ram_usage,get_ram_total());
                                write(_pipeRespond[1], buff, strlen(buff));
                                
                                //Swap                          
                                long swap_usage = get_swap_usage();
                                memset(buff, 0, sizeof(buff));
                                sprintf(buff, "Swap Usage: %ld\n", swap_usage);
                                write(_pipeRespond[1], buff, strlen(buff));
                                
                                //Dirs
                                char response[buffSize];
                                response[0] = '\0';

                                for (int i = 0; i < sett.countDirs; i++) {
									traceddir dir = sett.dirs[i];
									long directorySize = directory_size(dir.path);
									sprintf(response, "%sDirectory: %s, Size: %ld\n", response, dir.path, directorySize);
                                }

                                write(_pipeRespond[1], response, strlen(response));
                                
                                // Full tree
                                showTree(alarmTree, _pipeRespond[1],buffSize);
                                
                                break;
                            }
                            case 8:{
                                printStruct(sett, _pipeRespond[1],buffSize);
                            }
                            default: {
                                memset(buff, 0, buffSize);
                                sprintf(buff, "Неверная команда");
                                write(_pipeRespond[1], buff, strlen(buff));
                                break;
                            }
                        }
                        break;
                    }
                    case 1: { //clear
                        token=strtok(NULL,"|");
                        switch (atoi(token))
                        {
                        case 0: {
                            int id=atoi(strtok(NULL,"|"));
                            alarmTree=deleteItem(alarmTree, id);
                            break;
                        }
                        case 7: {
                            freeAlarmTree(alarmTree);
                            alarmTree = NULL;
                            highRamUsage = false;
                            highSwapUsage = false;
                            for(int i = 0; i < sett.countDirs; i++)
                                highDirectoryUsage[i] = false;
                            break;
                        }
                        default:{
                            memset(buff, 0, buffSize);
                            sprintf(buff, "Неверная команда");
                            write(_pipeRespond[1], buff, strlen(buff));
                            break;
                            }
                        }
                        break;
                    }
                    case 2: { //for config
						memset(buff, 0, sizeof(buff)); 
                        sprintf(buff, "Настройка завершена");
                        write(_pipeRespond[1], buff, strlen(buff));
						sett = parseSettings(); // чтобы обновить структуру
						break;
					}
                    case 3: { //exit
                        exitFlag = 1;
                        break;
                    }
                    default:{
                        break;
                    }
                }

                memset(buff, 0, sizeof(buff)); //конец ответа
                sprintf(buff, "\r"); //коретка используется как спец символ конца строки(\0 нельзя отправить по tcp)
                write(_pipeRespond[1], buff, strlen(buff));

                msgCount--;
                if (exitFlag) {
					break;
				}
            }
            sleep(1);
        }
        if(sett.answerRam) checkRamUsage();
        if(sett.answerSwap) checkSwapUsage();
        if(sett.answerDir) {
            for (int i = 0; i < sett.countDirs; i++) {
                checkDirectoryUsage(sett.dirs[i], &(highDirectoryUsage[i]));
            }
        }
        if (exitFlag) {
			break;
		}
    }
    exitSetupServer();
    return 0;
}

int startUpSetupClient() {
    openlog("memoger", LOG_PID, LOG_USER);
    sett = parseSettings();
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_CRIT, "Client socket creation failed");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(sett.port);
    return 0;
}

int exitSetupClient() {
    closelog();
    close(sockfd);
    close(connfd);
}

int commandConverter(char *com) {
    if (strcmp(com, "show") == 0) return 0;
    if (strcmp(com, "clear") == 0) return 1;
    if (strcmp(com, "config") == 0) return 2;
    if (strcmp(com, "exit") == 0) return 3;
    return -1;
}

int argumentComverter(char *arg) {
    if(strcmp(arg,"id") == 0)return 0;
    if(strcmp(arg,"ram") == 0)return 1;
    if(strcmp(arg,"swap") == 0)return 2;
    if(strcmp(arg,"dir") == 0)return 3;
    if(strcmp(arg,"port") == 0)return 4;
    if(strcmp(arg,"period") == 0)return 5;
    if(strcmp(arg,"tree") == 0)return 6;
    if(strcmp(arg,"all") == 0)return 7;
    if(strcmp(arg,"config") == 0)return 8;
    return -1;
}

int mainClient(int argc, char *argv[]) {
    char buff[buffSize] = {0};
    int com = commandConverter(argv[1]);
    if (com < 0) {
        printf("Неверная команда\n");
        return 1;
    }
    sprintf(buff, "%d", com);
    if (argc > 2) {
        int arg = argumentComverter(argv[2]);
        if (arg < 0 || com == 3) {
            printf("Неверный аргумент %s\n", argv[2]);
            return 1;
        }
        if(com == 2) {//config
            switch(arg) {
				case 7: {//all
					firstSet();
					break;
				}
                case 4: { //port
					editPort();
					break;
				}
				case 1: { //ram
					editRam();
					break;
				}
				case 2: { //swap
					editSwap();
					break;
				}
				case 3: { //dir
                    editDir();
					break;
				}
				case 5: { //period
					editFreq();
					break;
				}
                default: {
                    printf("Неверный аргумент %s\n", argv[2]);
                    return 1;
                }
            }
        }
        char buff2[buffSize] = {0};
        sprintf(buff2, "%s", buff);
        sprintf(buff, "%s|%d", buff2, arg);
        if (argc > 3) {
            char buff2[buffSize] = {0}, *end;
            int a = strtol(argv[3], &end, 10);
            if (strlen(end) != 0 || (arg != 0)) {
                printf("Неверный аргумент %s\n", argv[3]);
                return 1;
            }
            sprintf(buff2, "%s", buff);
            sprintf(buff, "%s|%d", buff2, a);
        }
    }else{
        if(com != 3){
            printf("Неверная комнанда\n");
            return 1;
        }
    }
    startUpSetupClient();
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        syslog(LOG_CRIT, "Client connection with the server failed");
        exit(0);
    }
    write(sockfd, buff, strlen(buff));
    while (1) {
        memset(buff, 0, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("%s", buff);
        if (buff[strlen(buff) - 1] == '\r') { 
			break;
		}
    }
    exitSetupClient();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Недостаточно аргументов\n");
        return EXIT_FAILURE;
    }
    if (argc > 4) {
        printf("Неверная последовательность аргументов\n");
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "start") == 0) {
		return mainServer(); //сервеная часть
	}
    else { //клиентская часть
        return mainClient(argc, argv);
    }
}
