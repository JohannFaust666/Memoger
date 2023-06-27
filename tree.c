#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

typedef struct TreeNode {
    int id;
    char timestamp[80];
    char message[100];
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

TreeNode* balanceTree(TreeNode*);

void showTree(TreeNode* node, int sock, int buffSize) {
    if (node == NULL) {
		return;
	}
    showTree(node->left, sock, buffSize);    
    char *buff = (char*)malloc(sizeof(char) * buffSize);
    memset(buff, 0, buffSize);
    sprintf(buff, "Alarm ID: %d\nTimestamp: %s\nMessage: %s\n----------------------\n", node->id, node->timestamp, node->message);
    write(sock, buff, strlen(buff));
    free(buff);
    showTree(node->right, sock, buffSize);
    return;
}
// Функция для создания нового узла аварии
TreeNode* createNode(int id, const char* timestamp, const char* message) {
    TreeNode* newNode = (TreeNode*)malloc(sizeof(TreeNode));
    newNode->id = id;
    strncpy(newNode->timestamp, timestamp, sizeof(newNode->timestamp));
    strncpy(newNode->message, message, sizeof(newNode->message));
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

// Функция для добавления аварии в дерево
TreeNode* addAlarm(TreeNode* root, int id, const char* timestamp, const char* message) {
    if (root == NULL) {
        return createNode(id, timestamp, message);
    }

    // Добавление аварии в правую или левую ветвь в зависимости от id
    if (id < root->id) {
        root->left = addAlarm(root->left, id, timestamp, message);
    } else {
        root->right = addAlarm(root->right, id, timestamp, message);
    }

    // Балансировка дерева
    TreeNode* nRoot = balanceTree(root);

    return nRoot;
}

TreeNode* findMinElement(TreeNode* root) {
    if (root->left == NULL) return root;

    return findMinElement(root->left);
}

TreeNode* deleteItem(TreeNode* root, int id) {
    if (root->id == id) {
        memset(root->message, 0, sizeof(root->message));
        memset(root->timestamp, 0, sizeof(root->timestamp));

        if (root->left == NULL && root->right == NULL) {
            return NULL;
        }

        if (root->left == NULL) {
            return root->right;
        }

        if (root->right == NULL) {
            return root->left;
        }

        TreeNode* minNodeInRightSubtree = findMinElement(root->right);
        root->id = minNodeInRightSubtree->id;
        strcpy(root->message, minNodeInRightSubtree->message);
        strcpy(root->timestamp, minNodeInRightSubtree->timestamp);

        root->right = deleteItem(root->right, minNodeInRightSubtree->id);
        return root;
    }

    if (id < root->id) {
        if (root->left == NULL) {
            syslog(LOG_WARNING, "Attempt to remove a non-existent element");
            return root;
        }

        root->left = deleteItem(root->left, id);

        return root;
    }

    if (id > root->id) {
        if (root->right == NULL) {
            syslog(LOG_WARNING, "Attempt to remove a non-existent element");
            return root;
        }

        root->right = deleteItem(root->right, id);
        return root;
    }
}

// Получение текущего времени в виде строки
void get_current_time(char* buffer) {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Функция для добавления аварии в дерево
TreeNode* addAlarmToTree(const char* message, int alarmId, TreeNode* alarmTree) {
    char timestamp[80];
    char alarmMessage[100];
    get_current_time(timestamp);
    snprintf(alarmMessage, sizeof(alarmMessage), message);
    alarmTree = addAlarm(alarmTree, alarmId, timestamp, alarmMessage);
    return alarmTree;
}

// Функция очистки памяти узлов дерева
void freeAlarmTree(TreeNode* root) {
    if (root != NULL) {
        freeAlarmTree(root->left);
        freeAlarmTree(root->right);
        free(root);
    }
}

// Функция для вычисления высоты узла дерева
int height(TreeNode* node) {
    if (node == NULL) {
        return 0;
    }
    int leftHeight = height(node->left);
    int rightHeight = height(node->right);
    if (leftHeight > rightHeight) {
		return (leftHeight + 1);
	}
	else {
		return (rightHeight + 1);
	}
}

// Функция для балансировки дерева
TreeNode* balanceTree(TreeNode* root) {
    if (root == NULL) {
        return NULL;
    }

    int balanceFactor = height(root->left) - height(root->right);

    // Левый поворот
    if (balanceFactor > 1) {
        if (height(root->left->left) >= height(root->left->right)) {
            // Простой левый поворот
            TreeNode* newRoot = root->left;
            root->left = newRoot->right;
            newRoot->right = root;
            return newRoot;
        } else {
            // Двойной левый поворот
            TreeNode* newRoot = root->left->right;
            root->left->right = newRoot->left;
            newRoot->left = root->left;
            root->left = newRoot->right;
            newRoot->right = root;
            return newRoot;
        }
    }

    // Правый поворот
    if (balanceFactor < -1) {
        if (height(root->right->right) >= height(root->right->left)) {
            // Простой правый поворот
            TreeNode* newRoot = root->right;
            root->right = newRoot->left;
            newRoot->left = root;
            return newRoot;
        } else {
            // Двойной правый поворот
            TreeNode* newRoot = root->right->left;
            root->right->left = newRoot->right;
            newRoot->right = root->right;
            root->right = newRoot->left;
            newRoot->left = root;
            return newRoot;
        }
    }

    return root;
}
