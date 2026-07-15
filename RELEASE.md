# Выпуск 4phone Windows

Публичные Windows-релизы создаются только workflow
`.github/workflows/release-windows.yml` по тегу. Локально собранные файлы не
публикуются как официальный релиз.

## Версии и каналы

Тег обязан точно совпадать с `v` и `_GLOBAL_VERSION` из
`microsip/const.h`.

- Beta: `_GLOBAL_VERSION` заканчивается на `-beta.N`, например
  `3.22.3-4phone.2-beta.1`. Workflow создает prerelease, файлы явно помечаются
  как `unsigned-beta`, а feed называется `appcast-beta.xml`.
- Stable: версия не содержит `beta`. Workflow требует две действительные
  Authenticode-подписи SignPath с timestamp и публикует `appcast.xml`.

`_GLOBAL_BUILD_VERSION` и `_GLOBAL_VERSION_COMMA` должны совпадать и содержать
четыре числовых компонента. WinSparkle сравнивает именно эту версию.

Release commit должен принадлежать ветке `main`. Повторная публикация
существующего тега запрещена.

## Защищенное GitHub environment

Workflow использует environment `release-signing`. В нем хранится только
секрет:

- `WINSPARKLE_ED25519_PRIVATE_KEY`

После одобрения проекта в SignPath Foundation нужно добавить:

- secret `SIGNPATH_API_TOKEN`;
- variable `SIGNPATH_ORGANIZATION_ID`;
- variable `SIGNPATH_PROJECT_SLUG`;
- variable `SIGNPATH_SIGNING_POLICY_SLUG`;
- variable `SIGNPATH_EXE_ARTIFACT_CONFIGURATION_SLUG`;
- variable `SIGNPATH_INSTALLER_ARTIFACT_CONFIGURATION_SLUG`.

Две Artifact Configuration нужны для отдельных запросов подписи
`4phone.exe` и `4phone-setup.exe`. Обе конфигурации должны разрешать только
ожидаемое имя файла. Signing Policy должна включать RFC 3161 timestamp.

Для environment включаются required reviewer, запрет self-review и доступ
только из защищенных тегов. У владельца репозитория и reviewer должна быть
включена MFA.

## Порядок stable-релиза

Workflow не позволяет менять порядок:

1. Собирает неподписанный `4phone.exe` из закрепленных pjproject и vcpkg.
2. Отправляет только EXE в SignPath.
3. Проверяет publisher `SignPath Foundation`, статус подписи и timestamp.
4. Устанавливает Inno Setup из закрепленного официального дистрибутива.
5. Собирает installer с уже подписанным EXE.
6. Отправляет только installer в SignPath и повторяет проверку.
7. Подписывает финальные байты installer ключом Ed25519 WinSparkle.
8. Проверяет Ed25519-подпись официальным `winsparkle-tool`.
9. Создает portable ZIP, appcast, SPDX SBOM и GitHub attestations.
10. Создает SHA-256 для всех приложенных файлов и публикует GitHub Release.

Версии и SHA-256 внешних release-инструментов закреплены в
`build/release-config.json`. Все сторонние GitHub Actions закреплены по
полному commit SHA.

## Первый beta-релиз

До одобрения SignPath разрешен только beta-тег. Перед созданием тега:

1. Обновить display и numeric версии в `microsip/const.h`.
2. Убедиться, что display-версия заканчивается на `-beta.N`.
3. Получить успешный обязательный CI на `main`.
4. Создать git-тег на проверенном commit.
5. Отправить тег в GitHub и подтвердить environment deployment.

Beta не содержит `appcast.xml`, поэтому backend stable-канала ее не
предлагает пользователям.

## Проверка релиза

На Windows:

```powershell
Get-AuthenticodeSignature .\4phone.exe
Get-AuthenticodeSignature .\4phone-setup.exe
Get-FileHash -Algorithm SHA256 .\4phone-setup.exe
```

Проверка provenance:

```powershell
gh attestation verify .\4phone-setup.exe `
  --repo XANDER-CAGE/4phone-windows-client
```

SHA-256 сверяется с `SHA256SUMS.txt`. SBOM должен проходить отчет
`SBOM-VALIDATION.json`. Stable appcast должен содержать `sparkle:version`,
`sparkle:shortVersionString`, точный размер installer и действительную
`sparkle:edSignature`.

## Ротация ключа WinSparkle

Закрытый ключ никогда не записывается в репозиторий или workflow-логи.
При плановой ротации новый public key одновременно обновляется в
`build/release-config.json` и `microsip/FourPhoneUpdateService.cpp`.
`build/validate-release-config.py` блокирует сборку при расхождении.

Компрометация ключа требует немедленного удаления environment secret,
остановки релизов и выпуска отдельной доверенной версии клиента с новым
public key.
