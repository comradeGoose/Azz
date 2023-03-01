# Azz

## Обзор

Программно-аппаратный комплекс управления школьными звонками (система оповещения) Azz.

This example designs several APIs to fetch resources as follows:

| API                        | Method | Resource Example                                      | Description                                                                              | Page URL |
| -------------------------- | ------ | ----------------------------------------------------- | ---------------------------------------------------------------------------------------- | -------- |
| `/`      | `GET`  | . . .       | . . .            | `/`      |

**Page URL** is the URL of the webpage which will send a request to the API.

### О фронтенд-фреймворке

В этом проекте использовался Vue 3, но можно использовать многие известные интерфейсные фреймворки (например React и Angular).

## Как пользоваться проектом

### Требуемое оборудование

Для запуска проекта потребудется аудиоплата на базе ESP32, RTC модуль (например DS1302), кабель **micro USB**, карта памяти.

### Конфигурация проекта

Использовать `sdkconfig` проекта.

### Сборка и прошивка

После создания веб-приложения его необходимо собрать:

```bash
cd path_to_this_example/front/web-demo
npm install
npm run build
```
Через некоторое время вы увидите папку `dist`, содержащую все файлы сайта (например, html, js, css, изображения).

Запустите `idf.py -p PORT flash monitor`, чтобы собрать и прошить проект.

(Чтобы выйти из монитора, введите ``Ctrl-]``.)

## Пример вывода

### ESP monitor output

```bash
I (. . .) . . .
```
