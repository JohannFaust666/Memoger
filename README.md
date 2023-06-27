<div align = "center">
![logo](Resources/icon.png)
<h1 align = "center">
[Главная](https://study-git.eltex-co.ru/alarm-manager-2023-june/alarm-manager) | [Документация](/docs) | [Discord](https://discord.com/channels/1016323666457403412/1114032161243275316)
</h1>
</div>

# Memoger
Memoger — это программа alarm-manager в формате демона, следящего за изменениями определенных параметров системы. В зависимости от значений, сохраняются и выводятся индикации о выходе за установленные пределы.

## Установка
Рекомендованный путь для установки Memoger из источника.
### Скачивание исходных файлов
```bash
git clone http://study-git.eltex-co.ru/alarm-manager-2023-june/alarm-manager.git
```
## Работа с Memoger
Полный список команд Вы можете найти в [документации](https://study-git.eltex-co.ru/alarm-manager-2023-june/alarm-manager/-/blob/bugsFixing/docs/Documentation.pdf) к программе.
Ниже приведены базовые команды для работы с Memoger: 

- Установка всех компонентов программы
```bash
make install
```
- Удаление всей программы и сопутствующих компонентов
```bash
make uninstall
```
- Запуск программы
```bash
memoger start
```
- Показ всех параметров
```bash
memoger show all
```
- Показ текущей конфигурации
```bash
memoger show config
```
- Очистка всех аварий
```bash
memoger clear
```
- Настройка всех параметров для работы программы
```bash
memoger config all 
```
- Закрытие программы
```bash
memoger exit
```

## RedMine проекта
[Проект](https://study-red.eltex-co.ru/projects/alarm-manager-2023-june)

## Документация
Документацию вместе с кратким руководством можно найти в каталоге [docs/](/docs). 

## Логи

Для просмотра логов программы введите в терминале:
```bash
 tail -f /var/log/syslog | grep memoger
 ```
Если видны не все логи или их вовсе нет, введите: 
```bash
cat /etc/syslog.conf
```
