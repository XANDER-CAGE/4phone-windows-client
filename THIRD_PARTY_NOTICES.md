# Уведомления о сторонних компонентах

4phone Windows является модифицированной версией MicroSIP и распространяется
по лицензии GPL-2.0-or-later. Полный текст лицензии находится в `LICENSE`.

## MicroSIP

- Версия основы: 3.22.3
- Авторские права: MicroSIP contributors, 2011-2025
- Источник: https://www.microsip.org/source
- Исходный архив: `MicroSIP-3.22.3-src.7z`
- SHA-256 исходного архива:
  `fd00f8c6d302d4a98825595c63880d23f986cbaca3652410cc4971d460740e98`
- Лицензия: GPL-2.0-or-later

Исходный архив не хранится в репозитории. Указанные URL, версия и хеш
обеспечивают проверяемую связь с исходной публикацией.

## pjproject

- Версия: 2.17
- Commit: `5a457451fa2712ba18e12b01738e8ff3af2b26fd`
- Источник: https://github.com/pjsip/pjproject
- Лицензия: GPL-2.0-or-later

## Opus

- Источник: https://opus-codec.org/
- Поставка: порт `opus` через зафиксированный baseline vcpkg
- Лицензия: BSD-3-Clause

## pugixml

- Источник: https://github.com/zeux/pugixml
- Поставка: порт `pugixml` через зафиксированный baseline vcpkg
- Авторские права: Arseny Kapoulkine
- Лицензия: MIT

## JsonCpp

- Источник: https://github.com/open-source-parsers/jsoncpp
- Поставка: исходники в `microsip/lib/jsoncpp`
- Лицензия: MIT или Public Domain, согласно уведомлениям upstream

## WinSparkle

- Источник: https://github.com/vslavik/winsparkle
- Поставка: порт `winsparkle` через зафиксированный baseline vcpkg
- Авторские права: Vaclav Slavik и участники проекта
- Лицензия: MIT

## Inno Setup

- Источник: https://jrsoftware.org/isinfo.php
- Используется только для создания установщика
- Лицензия: Inno Setup License

Двоичные релизы сопровождаются SBOM, который содержит точные версии разрешенных
зависимостей для конкретной сборки.
