
#include <fcntl.h>
#include <libconfig.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/sysinfo.h>
#include <sys/types.h>

static const char *output_file = "config.cfg";
config_t cfg;
config_setting_t *root, *setting, *group, *array, *settingDir, *settingDirIn;

typedef struct traceddir{
	char *path;
	long critical_size;
} traceddir;

typedef struct settings {
	int answerRam, answerSwap, answerDir;
	char *path[10];
	int limitRam, limitSwap, limitDir[10], port, countDirs, period;
	double hyster_ram, hyster_swap, hyster_dir;
	traceddir dirs[10];
} settings;

// открытие конфига
void openFile() { 
	root = config_root_setting(&cfg);
}

// конец работы с конфигом
void closeFile() {
	if (!config_write_file(&cfg, output_file)) // запись конфига в выходной файл
	{
		syslog(LOG_ERR, "Файл config.cfg не может закрыться");
		config_destroy(&cfg);
	}
	config_destroy(&cfg);
}

void readFile() {
	if (!config_read_file(&cfg, "config.cfg"))
	{
		syslog(LOG_ERR, "Ошибка чтения config.cfg файла");
		firstSet();
	}
}

// функция первоначальной настройки работы с частотой обращения программы
void setFreq() {
	int period;
	printf("Введите частоту отслеживания параметров программой в секундах: \n");
	scanf("%d", &period);
	group = config_setting_add(root, "frequency", CONFIG_TYPE_GROUP); // создание в конфиге группы port
	setting = config_setting_add(group, "period", CONFIG_TYPE_INT); // создание в конфиге типа number
	while (period < 1) {
		printf("Введите число больше. \n");
		scanf("%d", &period);
		syslog(LOG_NOTICE, "Пользователь ввел частоту опроса программы меньше 1.");
		if (period > 1) {
			break;
		}
	}
	config_setting_set_int(setting, period); // запись в тип number значения
}

void editFreq() {
	int period;
	config_setting_t *setting, *setting1;
	openFile();
	readFile();
	
	setting = config_lookup(&cfg, "frequency");
	if (setting != NULL) {
		printf("Введите частоту отслеживания параметров программой в секундах: \n");
		scanf("%d", &period);
		setting1 = config_lookup(&setting, "period");
		while (period < 1) {
			printf("Введите число больше. \n");
			scanf("%d", &period);
			syslog(LOG_NOTICE, "Пользователь ввел частоту опроса программы меньше 1.");
			if (period > 1) {
				break;
			}
		}
		config_setting_set_int(setting1, period); // запись в тип number значения
	}
	else {
		perror("Группы frequency не существует.");
	}
	closeFile();
}

// функция первоначальной настройки работы с портом
void setPort() {
	int portNumb;
	printf("Введите порт для работы с программой: \n");
	scanf("%d", &portNumb);
	group = config_setting_add(root, "port", CONFIG_TYPE_GROUP); // создание в конфиге группы port
	setting = config_setting_add(group, "number", CONFIG_TYPE_INT); // создание в конфиге типа number
	while (portNumb < 1024) {
		printf("Введите число больше. \n");
		scanf("%d", &portNumb);
		syslog(LOG_NOTICE, "Пользователь ввел порт меньше 1024.");
		if (portNumb > 1024) {
			break;
		}
	}
	config_setting_set_int(setting, portNumb); // запись в тип number значения
}

// функция изменения параметра порта
void editPort() {
	int portNumb;
	config_setting_t *setting, *setting1;
	openFile();
	readFile();
	
	setting = config_lookup(&cfg, "port");
	if (setting != NULL) {
		printf("Введите порт для работы с программой: \n");
		scanf("%d", &portNumb);
		setting1 = config_lookup(&setting, "number");
		while (portNumb < 1024) {
			printf("Введите число больше. \n");
			scanf("%d", &portNumb);
			syslog(LOG_NOTICE, "Пользователь ввел порт меньше 1024.");
			if (portNumb > 1024) {
				break;
			}
		}
		config_setting_set_int(setting1, portNumb); // запись в тип number значения
	}
	else {
		perror("Группы port не существует.");
	}
	closeFile();
}

// функция первоначальной настройки работы с ОЗУ
void setRam(int first) { 	
	char answerRam[3];
	int limitRam;
	float hyster_ram;
	struct sysinfo mem_info;
	sysinfo(&mem_info);
	
	if (first == 0) { // если это первоначальная настройка, то добавить параметры и поля
		group = config_setting_add(root, "ram", CONFIG_TYPE_GROUP);
		setting = config_setting_add(group, "check", CONFIG_TYPE_INT); // создание в конфиге типа
	} 
	else { // иначе проверить значения
		group = config_lookup(&cfg, "ram");
		setting = config_lookup(&group, "check");
	}
	
	printf("Хотите ли вы следить за ОЗУ? y/n \n");
	scanf("%s", answerRam);
	if (strcmp(answerRam, "y") == 0 || strcmp(answerRam, "yes") == 0) { // если введено y или yes
		config_setting_set_int(setting, 1); // запись в тип значения
		printf("Введите предел ОЗУ в байтах: \n");
		scanf("%d", &limitRam);	
		while (limitRam > (mem_info.totalram / (1024 * 1024)) ) {
			printf("Введите число меньше.\n");
			scanf("%d", &limitRam);	
			if (limitRam < (mem_info.totalram / (1024 * 1024)) ) {
				break;	
			}
		}
		printf("Введите коэффициент для нижнего порога гистерезиса: \n");
		scanf("%f", &hyster_ram);
		while (hyster_ram > 1) {
			printf("Введите число меньше 1.\n");
			scanf("%f", &hyster_ram);
			if (hyster_ram < 1) {
				break;
			}
		}
		setting = config_setting_add(group, "hyster", CONFIG_TYPE_FLOAT);
		config_setting_set_float(setting, hyster_ram);
		setting = config_setting_add(group, "limit", CONFIG_TYPE_INT); // создание в конфиге типа
		config_setting_set_int(setting, limitRam); // запись в тип значения
	}
	else {
		config_setting_set_int(setting, 0); // запись в тип значения
		syslog(LOG_NOTICE, "Пользователь отказался от проверки RAM.");
	}
}

// функция изменения параметров работы с ОЗУ
void editRam() {
	const char *str;
	int lim;
	config_setting_t *setting, *setting1;
	struct sysinfo mem_info;
	sysinfo(&mem_info);
	char answerRam[3];
	int limitRam;
	float hyster_ram;
	openFile();
	readFile();
	
	setting = config_lookup(&cfg, "ram");
	if (setting != NULL) {
		setting1 = config_lookup(&setting, "check");
		if (config_setting_get_int(setting1) != 0) { // если в конфиге значение стоит 1, то все поля уже есть
			printf("Хотите ли вы следить за ОЗУ? y/n \n");
			scanf("%s", answerRam);
			setting1 = config_lookup(&setting, "check");
			if (strcmp(answerRam, "y") == 0 || strcmp(answerRam, "yes") == 0) { // если введено y или yes
				config_setting_set_int(setting1, 1); // запись в тип значения
				printf("Введите предел ОЗУ в байтах: ");
				scanf("%d", &limitRam);	
				while (limitRam > (mem_info.totalram / (1024 * 1024)) ) {
					printf("Введите число меньше.\n");
					scanf("%d", &limitRam);	
					if (limitRam < (mem_info.totalram / (1024 * 1024)) ) {
						break;	
					}
				}
				setting1 = config_lookup(&setting, "limit");
				config_setting_set_int(setting1, limitRam); // запись в тип значения
				printf("Введите коэффициент для нижнего порога гистерезиса: \n");
				scanf("%f", &hyster_ram);
				while (hyster_ram > 1) {
					printf("Введите число меньше 1.\n");
					scanf("%f", &hyster_ram);
					if (hyster_ram < 1) {
						break;
					}
				}
				setting1 = config_lookup(&setting, "hyster");
				config_setting_set_float(setting1, hyster_ram);
			}
			else {
				config_setting_set_int(setting1, 0); // запись в тип значения
				syslog(LOG_NOTICE, "Пользователь отказался от проверки RAM.");
				if (config_setting_remove(setting, "limit") == NULL) {
					perror("ram.limit элемента нет.");
				}
				if (config_setting_remove(setting, "hyster") == NULL) {
					perror("ram.hyster элемента нет.");
				}
			}
		}
		else { // если в поле check стоит 0, то нет всех полей и нужно провести первоначальную настройку
			setRam(1);
		}
	}
	else {
		perror("Группы ram не существует.");
	}
	closeFile();
}

// функция настройки работы с файлом подкачки
void setSwap(int first) {
	char answerSwap[3];
	int limitSwap;
	float hyster_swap;
	struct sysinfo mem_info;
	sysinfo(&mem_info);
	
	if (first == 0) { // если это первоначальная настройка, то добавить параметры и поля
		group = config_setting_add(root, "swap", CONFIG_TYPE_GROUP);
		setting = config_setting_add(group, "check", CONFIG_TYPE_INT); // создание в конфиге типа
	} 
	else { // иначе проверить значения
		group = config_lookup(&cfg, "swap");
		setting = config_lookup(&group, "check");
	}
	
	printf("Хотите ли вы следить за SWAP? y/n \n");
	scanf("%s", answerSwap);
	if (strcmp(answerSwap, "y") == 0 || strcmp(answerSwap, "yes") == 0) { // если введено y или yes
		config_setting_set_int(setting, 1); // запись в тип значения
		printf("Введите предел SWAP в байтах: \n");
		scanf("%d", &limitSwap);
		while (limitSwap > (mem_info.totalswap / (1024 * 1024)) ) {
				printf("Введите число меньше.\n");
				scanf("%d", &limitSwap);
				if (limitSwap < (mem_info.totalswap / (1024 * 1024)) ) {
					break;	
				}
		}
		printf("Введите коэффициент для нижнего порога гистерезиса: \n");
		scanf("%f", &hyster_swap);
		while (hyster_swap > 1) {
			printf("Введите число меньше 1.\n");
			scanf("%f", &hyster_swap);
			if (hyster_swap < 1) {
				break;
			}
		}
		setting = config_setting_add(group, "hyster", CONFIG_TYPE_FLOAT);
		config_setting_set_float(setting, hyster_swap);
		setting = config_setting_add(group, "limit", CONFIG_TYPE_INT); // создание в конфиге типа
		config_setting_set_int(setting, limitSwap); // запись в тип значения
	}
	else {
		config_setting_set_int(setting, 0); // запись в тип значения
		syslog(LOG_NOTICE, "Пользователь отказался от проверки СВАП.");
	}
}

void editSwap() {
	const char *str;
	int lim;
	config_setting_t *setting, *setting1;
	struct sysinfo mem_info;
	sysinfo(&mem_info);
	char answerSwap[3];
	int limitSwap;
	float hyster_swap;
	openFile();
	readFile();
	
	setting = config_lookup(&cfg, "swap");
	if (setting != NULL) {
		setting1 = config_lookup(&setting, "check");
		if (config_setting_get_int(setting1) != 0) { // если в конфиге значение стоит 1, то все поля уже есть
			printf("Хотите ли вы следить за SWAP? y/n \n");
			scanf("%s", answerSwap);
			setting1 = config_lookup(&setting, "check");
			if (strcmp(answerSwap, "y") == 0 || strcmp(answerSwap, "yes") == 0) { // если введено y или yes
				config_setting_set_int(setting1, 1); // запись в тип значения
				printf("Введите предел SWAP в байтах: \n");
				scanf("%d", &limitSwap);	
				while (limitSwap > (mem_info.totalswap / (1024 * 1024)) ) {
					printf("Введите число меньше.\n");
					scanf("%d", &limitSwap);	
					if (limitSwap < (mem_info.totalswap / (1024 * 1024)) ) {
						break;	
					}
				}
				printf("Введите коэффициент для нижнего порога гистерезиса: \n");
				scanf("%f", &hyster_swap);
				while (hyster_swap > 1) {
					printf("Введите число меньше 1.\n");
					scanf("%f", &hyster_swap);
					if (hyster_swap < 1) {
						break;
					}
				}
				setting1 = config_lookup(&setting, "hyster");
				config_setting_set_float(setting1, hyster_swap);
				setting1 = config_lookup(&setting, "limit");
				config_setting_set_int(setting1, limitSwap); // запись в тип значения
			}
			else {
				config_setting_set_int(setting1, 0); // запись в тип значения
				syslog(LOG_NOTICE, "Пользователь отказался от проверки СВАП.");
				if (config_setting_remove(setting, "limit") == NULL){
					perror("swap.limit элемента нет.");
				}
				if (config_setting_remove(setting, "hyster") == NULL){
					perror("swap.hyster элемента нет.");
				}
			}
		}
		else { // если в поле check стоит 0, то нет всех полей и нужно провести первоначальную настройку
			setSwap(1);
		}
	}
	else {
		perror("Группы swap не существует.");
	}
	closeFile();
}

// функция настройки работы с директориями
void setDir(int first) { 
	char answerDir[3];
	int numberDir;
	float hyster_dir;
	
	if (first == 0) { // если это первоначальная настройка, то добавить параметры и поля
		group = config_setting_add(root, "dir", CONFIG_TYPE_GROUP);
		setting = config_setting_add(group, "check", CONFIG_TYPE_INT); // создание в конфиге типа
	} 
	else { // иначе проверить значения
		group = config_lookup(&cfg, "dir");
		setting = config_lookup(&group, "check");
	}
	
	printf("Хотите ли вы следить за памятью директорий? y/n \n");
	scanf("%s", answerDir);
	if (strcmp(answerDir, "y") == 0 || strcmp(answerDir, "yes") == 0) { // если введено y или yes
		config_setting_set_int(setting, 1); // запись в тип значения
		printf("Введите количесто директорий: \n");
		scanf("%d", &numberDir);
		group = config_lookup(&cfg, "dir");
		setting = config_setting_add(group, "countDirs", CONFIG_TYPE_INT);
		config_setting_set_int(setting, numberDir); // запись в тип значения
		settingDir = config_setting_add(group, "dirs", CONFIG_TYPE_LIST);
		for (int i = 1; i <= numberDir; i++) {
			settingDirIn = config_setting_add(settingDir, NULL, CONFIG_TYPE_GROUP); 
			char path[100];
			int limitDir;
			printf("Введите путь директории %d: \n", i);
			scanf("%s", path);
			setting = config_setting_add(settingDirIn, "path", CONFIG_TYPE_STRING); 
			config_setting_set_string(setting, path); 
			
			printf("Введите допустимый размер директории в байтах: \n");
			scanf("%d", &limitDir);
			setting = config_setting_add(settingDirIn, "limit", CONFIG_TYPE_INT); 
			config_setting_set_int(setting, limitDir);
		}
		printf("Введите коэффициент для нижнего порога гистерезиса: \n");
		scanf("%f", &hyster_dir);
		while(hyster_dir > 1) {
			printf("Введите число меньше 1.\n");
			scanf("%f", &hyster_dir);
			if (hyster_dir < 1) {
				break;
			}
		}
		setting = config_setting_add(group, "hyster", CONFIG_TYPE_FLOAT);
		config_setting_set_float(setting, hyster_dir);
	}
	else {
		config_setting_set_int(setting, 0); // запись в тип значения
		syslog(LOG_NOTICE, "Пользователь отказался от проверки директорий.");
	}
}

void editDir() {
	const char *str;
	int lim;
	config_setting_t *setting, *setting1;
	char answerDir[3];
	int numberDir;
	float hyster_dir;
	openFile();
	readFile();
	
	setting = config_lookup(&cfg, "dir");
	if (setting != NULL) {
		setting1 = config_lookup(&setting, "check"); // получаем значение поля check 
		if (config_setting_get_int(setting1) != 0) { // если в конфиге значение стоит 1, то все поля уже есть
			printf("Хотите ли вы следить за памятью директорий? y/n \n");
			scanf("%s", answerDir);
			if (strcmp(answerDir, "y") == 0 || strcmp(answerDir, "yes") == 0) { // если введено y или yes
				config_setting_set_int(setting1, 1); // запись в тип значения
				printf("Введите количесто директорий: \n");
				scanf("%d", &numberDir);
				setting1 = config_lookup(&setting, "countDirs");
				config_setting_set_int(setting1, numberDir); // запись нового значения
				config_setting_remove(setting, "dirs"); // удаление всех записей для новых
				setting1 = config_setting_add(setting, "dirs", CONFIG_TYPE_LIST); // создание группы заново
				for (int i = 1; i <= numberDir; i++) {
					settingDirIn = config_setting_add(setting1, NULL, CONFIG_TYPE_GROUP); 
					char path[100];
					int limitDir;
					printf("Введите путь директории %d: \n", i);
					scanf("%s", path);
					setting = config_setting_add(settingDirIn, "path", CONFIG_TYPE_STRING); 
					config_setting_set_string(setting, path); 
			
					printf("Введите допустимый размер директории в байтах: \n");
					scanf("%d", &limitDir);
					setting = config_setting_add(settingDirIn, "limit", CONFIG_TYPE_INT);
					config_setting_set_int(setting, limitDir);
				}
				printf("Введите коэффициент для нижнего порога гистерезиса: \n");
				scanf("%f", &hyster_dir);
				while(hyster_dir > 1) {
					printf("Введите число меньше 1.\n");
					scanf("%f", &hyster_dir);
					if (hyster_dir < 1) {
						break;
					}
				}
				setting = config_lookup(&cfg, "dir");
				setting1 = config_lookup(&setting, "hyster");
				config_setting_set_float(setting1, hyster_dir);
			}
			else {
				setting1 = config_lookup(&setting, "check");
				config_setting_set_int(setting1, 0); // запись в тип значения
				syslog(LOG_NOTICE, "Пользователь отказался от проверки директорий.");
				if (config_setting_remove(setting, "countDirs") == NULL) {
						perror("dir.countDirs элемента нет.");
				}
				if (config_setting_remove(setting, "dirs") == NULL) {
						perror("dir.dirs элемента нет.");
				}
				if (config_setting_remove(setting, "hyster") == NULL) {
						perror("dir.hyster элемента нет.");
				}
			}
		}
		else { // если в поле check стоит 0, то нет всех полей и нужно провести первоначальную настройку
			setDir(1);	
		}
	}
	else {
		perror("Группы dir не существует.");
	}
	closeFile();
}

// функция полной настройки, заполняется пользователем
void firstSet() {
	config_init(&cfg); // инициализация конфига, сбрасывает прошлый конфиг
	openFile();
	setPort();
	setFreq();
	setRam(0);
	setSwap(0);
	setDir(0);
	closeFile();
	syslog(LOG_NOTICE, "Новая конфигурация успешно записана в config.cfg.");
}

// функция чтения данных из конфига, записывает данные в структуру
settings parseSettings() {
	config_setting_t *setting, *settingIn;
	settings sett;
	
	if (!config_read_file(&cfg, "config.cfg"))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		syslog(LOG_CRIT, "Невозможно прочитать config.cfg.");
		return sett;
	}
	
	setting = config_lookup(&cfg, "frequency");
	if (setting != NULL) {
		int qty;
		config_setting_lookup_int(setting, "period", &qty);
		sett.period = qty;
	}
	
	setting = config_lookup(&cfg, "port");
	if (setting != NULL) {
		int qty;
		config_setting_lookup_int(setting, "number", &qty);
		sett.port = qty;
	}
	
	setting = config_lookup(&cfg, "ram");
	if (setting != NULL) {
		int str;
		config_setting_lookup_int(setting, "check", &str);
		sett.answerRam = str;
		int lim;
		config_setting_lookup_int(setting, "limit", &lim);
		sett.limitRam = lim;
		double hyster;
		config_setting_lookup_float(setting, "hyster", &hyster);
		sett.hyster_ram = hyster;
	}
	
	setting = config_lookup(&cfg, "swap");
	if (setting != NULL) {
		int str;
		config_setting_lookup_int(setting, "check", &str);
		sett.answerSwap = str;
		int lim;
		config_setting_lookup_int(setting, "limit", &lim);
		sett.limitSwap = lim;
		double hyster;
		config_setting_lookup_float(setting, "hyster", &hyster);
		sett.hyster_swap = hyster;
	}
	
	setting = config_lookup(&cfg, "dir");
	if (setting != NULL) {
		int str;
		config_setting_lookup_int(setting, "check", &str);
		sett.answerDir = str;
		int lim;
		config_setting_lookup_int(setting, "countDirs", &lim);
		sett.countDirs = lim;
		double hyster;
		config_setting_lookup_float(setting, "hyster", &hyster);
		sett.hyster_dir = hyster;
	}
	
	settingIn = config_lookup(&cfg, "dir.dirs");
	
	if (settingIn != NULL) {
		for (int i = 0; i < sett.countDirs; i++){
			config_setting_t *dirs = config_setting_get_elem(settingIn, i);
			const char *str1;
			int lim1;
			config_setting_lookup_string(dirs, "path", &str1);
			sett.dirs[i].path = str1;
			config_setting_lookup_int(dirs, "limit", &lim1);
			sett.dirs[i].critical_size = lim1;
		}
	}
	return sett;
}

// функция для вывода данных структуры
void printStruct(settings sett, int sock, int buffSize) {
	char *buff = (char*)malloc(sizeof(char) * buffSize);

	memset(buff, 0, buffSize);
	sprintf(buff, "Port: %d\n", sett.port);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Freq period: %d\n", sett.period);
	write(sock, buff, strlen(buff));
	
	memset(buff, 0, buffSize);
	sprintf(buff, "Answer Ram: %d\n", sett.answerRam);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Limit Ram: %d\n", sett.limitRam);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Hysteresis Ram: %lf\n", sett.hyster_ram);
	write(sock, buff, strlen(buff));
	
	memset(buff, 0, buffSize);
	sprintf(buff, "Answer Ram: %d\n", sett.answerSwap);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Limit Swap: %d\n", sett.limitSwap);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Hysteresis Swap: %lf\n", sett.hyster_swap);
	write(sock, buff, strlen(buff));
	
	memset(buff, 0, buffSize);
	sprintf(buff, "Answer Dir: %d\n", sett.answerDir);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Count Dirs: %d\n", sett.countDirs);
	write(sock, buff, strlen(buff));
	memset(buff, 0, buffSize);
	sprintf(buff, "Hysteresis Dir: %lf\n", sett.hyster_dir);
	write(sock, buff, strlen(buff));
	for (int i = 0; i < sett.countDirs; i++) {
		memset(buff, 0, buffSize);
		sprintf(buff, "Path %d: %s \t", i+1, sett.dirs[i].path);
		write(sock, buff, strlen(buff));
		memset(buff, 0, buffSize);
		sprintf(buff, "Limit %d: %ld \n", i+1, sett.dirs[i].critical_size);
		write(sock, buff, strlen(buff));
	}
}
