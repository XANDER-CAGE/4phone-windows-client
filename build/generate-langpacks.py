#!/usr/bin/env python3
"""Generate the bundled 4phone language packs from MicroSIP translations."""

from __future__ import annotations

import html
from collections import OrderedDict
from html.parser import HTMLParser
from pathlib import Path
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "microsip" / "langpacks"

LANGUAGES = {
    "ru": {
        "translation_id": 41,
        "filename": "langpack_russian.txt",
        "name": "Russian",
        "locale": "0419",
    },
    "uz": {
        "translation_id": 54,
        "filename": "langpack_uzbek.txt",
        "name": "Uzbek",
        "locale": "0443",
    },
}

CUSTOM_TRANSLATIONS = {
    "ru": {
        "Sign in to 4phone": "Вход в 4phone",
        "Verify your identity": "Подтвердите личность",
        "Choose an extension": "Выберите добавочный номер",
        "2FA code": "Код 2FA",
        "2FA code or recovery code": "Код 2FA или резервный код",
        "Extension": "Добавочный номер",
        "Enter your 4phone email and password": "Введите email и пароль от личного кабинета 4phone",
        "Enter email and password": "Введите email и пароль",
        "Checking your account...": "Проверяем учетную запись...",
        "Enter the code from your authenticator app": "Введите код из приложения-аутентификатора",
        "Enter verification code": "Введите код подтверждения",
        "Checking code...": "Проверяем код...",
        "Loading telephony settings...": "Получаем настройки телефонии...",
        "Loading available extensions...": "Получаем доступные добавочные...",
        "No active extension is assigned to your user": "К пользователю не привязан активный добавочный номер",
        "Applying telephony settings...": "Применяем настройки телефонии...",
        "Select an extension for this computer": "Выберите добавочный для этого компьютера",
        "Select an extension": "Выберите добавочный номер",
        "No extension selected": "Добавочный номер не выбран",
        "Connect": "Подключить",
        "Sign in": "Войти",
        "Please wait...": "Подождите...",
        "The server returned an invalid response": "Сервер вернул некорректный ответ",
        "The server did not return a 2FA challenge token": "Сервер не вернул токен проверки 2FA",
        "The server returned an invalid extension list": "Сервер вернул некорректный список добавочных",
        "The server returned an invalid SIP configuration": "Сервер вернул некорректную SIP-конфигурацию",
        "The SIP configuration is missing required fields": "В SIP-конфигурации отсутствуют обязательные поля",
        "Your 4phone session is not authorized": "Сеанс 4phone не авторизован",
        "The 4phone API must use HTTPS": "API 4phone должен использовать HTTPS",
        "The 4phone API response is too large": "Ответ API 4phone слишком большой",
        "The server returned an invalid authentication response": "Сервер вернул некорректный ответ авторизации",
        "The server did not return authentication tokens": "Сервер не вернул токены авторизации",
        "Invalid email, password, or verification code": "Неверный email, пароль или код подтверждения",
        "Check the entered data and try again": "Проверьте введенные данные и повторите попытку",
        "Access to this extension is denied": "Доступ к добавочному запрещен",
        "The selected extension is no longer available": "Выбранный добавочный больше недоступен",
        "Too many attempts. Try again later": "Слишком много попыток. Повторите позже",
        "4phone is temporarily unavailable. Try again later": "4phone временно недоступен. Повторите позже",
        "The 4phone API returned HTTP error %lu": "API 4phone вернул ошибку HTTP %lu",
        "Could not connect to the 4phone API, error %lu": "Не удалось подключиться к API 4phone, код %lu",
        "Could not connect to the 4phone API: %s": "Не удалось подключиться к API 4phone: %s",
        "Your 4phone session has ended": "Сеанс 4phone завершен",
        "Windows could not protect the SIP password. Settings were not saved": "Windows не смог защитить SIP-пароль. Настройки не сохранены",
        "Could not save the 4phone sign-in state": "Не удалось сохранить состояние входа 4phone",
        "Could not save the local sign-out state": "Не удалось сохранить локальный признак выхода",
        "Cannot save an empty 4phone token": "Нельзя сохранить пустой токен 4phone",
        "4phone token exceeds the Windows credential size limit": "Токен 4phone превышает лимит хранилища Windows",
        "Could not save the 4phone session": "Не удалось сохранить сеанс 4phone",
        "Could not read the 4phone session": "Не удалось прочитать сеанс 4phone",
        "Saved 4phone session is corrupted": "Сохраненный сеанс 4phone поврежден",
        "Could not delete the 4phone session": "Не удалось удалить сеанс 4phone",
        ", error %lu": ", код %lu",
        "Sign out from 4phone and remove telephony settings from this computer?": "Выйти из 4phone и удалить настройки телефонии на этом компьютере?",
        "Switch 4phone account...": "Сменить учетную запись 4phone...",
        "Sign in to 4phone...": "Войти в 4phone...",
        "Sign out from 4phone": "Выйти из 4phone",
        "Automatic 4phone client updates are not configured yet": "Автоматическое обновление клиента 4phone пока не настроено",
        "4phone encountered an error. Diagnostic file saved to: %s": "В 4phone произошла ошибка. Диагностический файл сохранен: %s",
        "Interface language": "Язык интерфейса",
        "Check for updates": "Проверять обновления",
        "Updates": "Обновления",
        "Every launch": "При каждом запуске",
        "Every day": "Каждый день",
        "Every week": "Каждую неделю",
        "Every month": "Каждый месяц",
        "Every three months": "Каждые три месяца",
        "The interface language will change after restarting 4phone.": "Язык интерфейса изменится после перезапуска 4phone.",
        "Secure cloud telephony": "Защищенная облачная телефония",
        "Basic settings": "Основные настройки",
        "Advanced settings": "Дополнительные настройки",
        "Daily call settings. SIP and network controls are under Advanced settings.": "Повседневные параметры звонков. Настройки SIP и сети находятся в дополнительных настройках.",
        "Keep everyday options here. Open advanced settings for SIP and network controls.": "Здесь собраны повседневные параметры. Настройки SIP и сети находятся в дополнительных настройках.",
        "Audio": "Аудио",
        "Audio devices": "Аудиоустройства",
        "Ringing": "Мелодия звонка",
        "Ring device": "Устройство звонка",
        "Speakers": "Динамики",
        "Microphone": "Микрофон",
        "Language": "Язык",
        "Behavior": "Поведение",
        "Application behavior": "Поведение приложения",
        "Use one call window": "Использовать одно окно звонка",
        "Bring incoming call to front": "Выводить входящий звонок на передний план",
        "Allow call waiting": "Разрешить второй звонок",
        "Start 4phone with Windows": "Запускать 4phone вместе с Windows",
        "System default": "Системное значение",
        "Minimize": "Свернуть",
        "Maximize": "Развернуть",
        "Restore": "Восстановить",
        "Close": "Закрыть",
        "AA": "AA",
        "AC": "AC",
        "Allow access to the microphone in your antivirus settings.": "Разрешите доступ к микрофону в настройках антивируса.",
        "CONF": "CONF",
        "Contact with the same number already exists. Do you want to overwrite?": "Контакт с таким номером уже существует. Перезаписать?",
        "DND": "DND",
        "Delete contact": "Удалить контакт",
        "Diversion": "Переадресация",
        "FWD": "FWD",
        "Final": "Финальный",
        "Fix microphone permission in the Windows settings (Windows Settings => Privacy => Microphone).": "Разрешите доступ к микрофону в параметрах Windows (Параметры => Конфиденциальность => Микрофон).",
        "In-band": "В аудиопотоке",
        "Info": "Информация",
        "REC": "REC",
        "RFC2833": "RFC2833",
        "SIP-INFO": "SIP-INFO",
        "Speakers and microphone both are required. To make calls you must have input and output sound device in your system.": "Для звонков необходимы динамики и микрофон. Убедитесь, что в системе доступны устройства ввода и вывода звука.",
        "Symmetric NAT detected!": "Обнаружен симметричный NAT!",
        "Transfer Call To": "Перевести вызов на",
        "Send": "Отправить",
    },
    "uz": {
        "Sign in to 4phone": "4phone tizimiga kirish",
        "Verify your identity": "Shaxsingizni tasdiqlang",
        "Choose an extension": "Ichki raqamni tanlang",
        "2FA code": "2FA kodi",
        "Password": "Parol",
        "2FA code or recovery code": "2FA kodi yoki tiklash kodi",
        "Extension": "Ichki raqam",
        "Cancel": "Bekor qilish",
        "Enter your 4phone email and password": "4phone shaxsiy kabinetidagi email va parolni kiriting",
        "Enter email and password": "Email va parolni kiriting",
        "Checking your account...": "Hisob tekshirilmoqda...",
        "Enter the code from your authenticator app": "Autentifikator ilovasidagi kodni kiriting",
        "Enter verification code": "Tasdiqlash kodini kiriting",
        "Checking code...": "Kod tekshirilmoqda...",
        "Loading telephony settings...": "Telefoniya sozlamalari olinmoqda...",
        "Loading available extensions...": "Mavjud ichki raqamlar olinmoqda...",
        "No active extension is assigned to your user": "Foydalanuvchiga faol ichki raqam biriktirilmagan",
        "Applying telephony settings...": "Telefoniya sozlamalari qo'llanmoqda...",
        "Select an extension for this computer": "Ushbu kompyuter uchun ichki raqamni tanlang",
        "Select an extension": "Ichki raqamni tanlang",
        "No extension selected": "Ichki raqam tanlanmagan",
        "Connect": "Ulash",
        "Sign in": "Kirish",
        "Please wait...": "Kuting...",
        "The server returned an invalid response": "Server noto'g'ri javob qaytardi",
        "The server did not return a 2FA challenge token": "Server 2FA tekshiruv tokenini qaytarmadi",
        "The server returned an invalid extension list": "Server ichki raqamlarning noto'g'ri ro'yxatini qaytardi",
        "The server returned an invalid SIP configuration": "Server noto'g'ri SIP konfiguratsiyasini qaytardi",
        "The SIP configuration is missing required fields": "SIP konfiguratsiyasida majburiy maydonlar yo'q",
        "Your 4phone session is not authorized": "4phone seansi avtorizatsiyadan o'tmagan",
        "The 4phone API must use HTTPS": "4phone API HTTPS orqali ishlashi kerak",
        "The 4phone API response is too large": "4phone API javobi juda katta",
        "The server returned an invalid authentication response": "Server autentifikatsiya uchun noto'g'ri javob qaytardi",
        "The server did not return authentication tokens": "Server autentifikatsiya tokenlarini qaytarmadi",
        "Invalid email, password, or verification code": "Email, parol yoki tasdiqlash kodi noto'g'ri",
        "Check the entered data and try again": "Kiritilgan ma'lumotlarni tekshirib, qayta urinib ko'ring",
        "Access to this extension is denied": "Ushbu ichki raqamga kirish taqiqlangan",
        "The selected extension is no longer available": "Tanlangan ichki raqam boshqa mavjud emas",
        "Too many attempts. Try again later": "Urinishlar juda ko'p. Keyinroq qayta urinib ko'ring",
        "4phone is temporarily unavailable. Try again later": "4phone vaqtincha ishlamayapti. Keyinroq qayta urinib ko'ring",
        "The 4phone API returned HTTP error %lu": "4phone API HTTP %lu xatosini qaytardi",
        "Could not connect to the 4phone API, error %lu": "4phone API ga ulanib bo'lmadi, xato %lu",
        "Could not connect to the 4phone API: %s": "4phone API ga ulanib bo'lmadi: %s",
        "Your 4phone session has ended": "4phone seansi yakunlandi",
        "Windows could not protect the SIP password. Settings were not saved": "Windows SIP parolini himoyalay olmadi. Sozlamalar saqlanmadi",
        "Could not save the 4phone sign-in state": "4phone kirish holatini saqlab bo'lmadi",
        "Could not save the local sign-out state": "Mahalliy chiqish holatini saqlab bo'lmadi",
        "Cannot save an empty 4phone token": "Bo'sh 4phone tokenini saqlab bo'lmaydi",
        "4phone token exceeds the Windows credential size limit": "4phone tokeni Windows hisob ma'lumotlari hajmi limitidan oshadi",
        "Could not save the 4phone session": "4phone seansini saqlab bo'lmadi",
        "Could not read the 4phone session": "4phone seansini o'qib bo'lmadi",
        "Saved 4phone session is corrupted": "Saqlangan 4phone seansi buzilgan",
        "Could not delete the 4phone session": "4phone seansini o'chirib bo'lmadi",
        ", error %lu": ", xato %lu",
        "Sign out from 4phone and remove telephony settings from this computer?": "4phone tizimidan chiqib, ushbu kompyuterdagi telefoniya sozlamalari o'chirilsinmi?",
        "Switch 4phone account...": "4phone hisobini almashtirish...",
        "Sign in to 4phone...": "4phone tizimiga kirish...",
        "Sign out from 4phone": "4phone tizimidan chiqish",
        "Automatic 4phone client updates are not configured yet": "4phone klientining avtomatik yangilanishi hali sozlanmagan",
        "4phone encountered an error. Diagnostic file saved to: %s": "4phone da xato yuz berdi. Diagnostika fayli saqlandi: %s",
        "Interface language": "Interfeys tili",
        "Check for updates": "Yangilanishlarni tekshirish",
        "Updates": "Yangilanishlar",
        "Every launch": "Har ishga tushganda",
        "Every day": "Har kuni",
        "Every week": "Har hafta",
        "Every month": "Har oy",
        "Every three months": "Har uch oyda",
        "The interface language will change after restarting 4phone.": "Interfeys tili 4phone qayta ishga tushirilgandan keyin o'zgaradi.",
        "Secure cloud telephony": "Xavfsiz bulutli telefoniya",
        "Basic settings": "Asosiy sozlamalar",
        "Advanced settings": "Kengaytirilgan sozlamalar",
        "Daily call settings. SIP and network controls are under Advanced settings.": "Kundalik qo'ng'iroq sozlamalari. SIP va tarmoq sozlamalari kengaytirilgan bo'limda.",
        "Keep everyday options here. Open advanced settings for SIP and network controls.": "Kundalik parametrlar shu yerda. SIP va tarmoq sozlamalari kengaytirilgan bo'limda.",
        "Audio": "Audio",
        "Audio devices": "Audio qurilmalar",
        "Ringing": "Qo'ng'iroq ohangi",
        "Ring device": "Qo'ng'iroq qurilmasi",
        "Speakers": "Karnaylar",
        "Microphone": "Mikrofon",
        "Language": "Til",
        "Behavior": "Ishlash tartibi",
        "Application behavior": "Ilova ishlashi",
        "Use one call window": "Bitta qo'ng'iroq oynasidan foydalanish",
        "Bring incoming call to front": "Kiruvchi qo'ng'iroqni oldinga chiqarish",
        "Allow call waiting": "Qo'ng'iroqni kutishga ruxsat berish",
        "Start 4phone with Windows": "Windows bilan birga 4phone ni ishga tushirish",
        "System default": "Tizim standarti",
        "Minimize": "Yig'ish",
        "Maximize": "Kattalashtirish",
        "Restore": "Tiklash",
        "Close": "Yopish",
        "AA": "AA",
        "AC": "AC",
        "Allow access to the microphone in your antivirus settings.": "Antivirus sozlamalarida mikrofonga kirishga ruxsat bering.",
        "CONF": "CONF",
        "Contact with the same number already exists. Do you want to overwrite?": "Bunday raqamli kontakt mavjud. Uni almashtirasizmi?",
        "DND": "DND",
        "Delete contact": "Kontaktni o'chirish",
        "Diversion": "Yo'naltirish",
        "FWD": "FWD",
        "Final": "Yakuniy",
        "Fix microphone permission in the Windows settings (Windows Settings => Privacy => Microphone).": "Windows sozlamalarida mikrofonga ruxsat bering (Sozlamalar => Maxfiylik => Mikrofon).",
        "In-band": "Audio oqimida",
        "Info": "Ma'lumot",
        "REC": "REC",
        "RFC2833": "RFC2833",
        "SIP-INFO": "SIP-INFO",
        "Speakers and microphone both are required. To make calls you must have input and output sound device in your system.": "Qo'ng'iroqlar uchun karnay va mikrofon kerak. Tizimda ovoz kiritish va chiqarish qurilmalari mavjudligini tekshiring.",
        "Symmetric NAT detected!": "Simmetrik NAT aniqlandi!",
        "Transfer Call To": "Qo'ng'iroqni o'tkazish",
        "Send": "Yuborish",
    },
}


class TranslationParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.translate_depth = 0
        self.pending_source: str | None = None
        self.translations: OrderedDict[str, str] = OrderedDict()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attributes = dict(attrs)
        if tag == "div":
            classes = (attributes.get("class") or "").split()
            if self.translate_depth > 0:
                self.translate_depth += 1
            elif "Translate" in classes:
                self.translate_depth = 1
            return

        if tag != "input" or self.translate_depth == 0:
            return

        value = html.unescape(attributes.get("value") or "").strip()
        if "readonly" in attributes:
            self.pending_source = value
            return

        element_id = attributes.get("id") or ""
        if element_id.startswith("t") and self.pending_source:
            if value:
                self.translations[self.pending_source] = value
            self.pending_source = None

    def handle_endtag(self, tag: str) -> None:
        if tag == "div" and self.translate_depth > 0:
            self.translate_depth -= 1


def fetch_translations(translation_id: int) -> OrderedDict[str, str]:
    request = Request(
        f"https://www.microsip.org/translation/translate/{translation_id}",
        headers={"User-Agent": "4phone-langpack-builder/1.0"},
    )
    with urlopen(request, timeout=30) as response:
        body = response.read().decode("utf-8")

    parser = TranslationParser()
    parser.feed(body)
    if len(parser.translations) < 250:
        raise RuntimeError(
            f"Only {len(parser.translations)} translations were found for language {translation_id}"
        )
    return parser.translations


def escape_langpack_value(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace("[", "\\[")
        .replace("]", "\\]")
        .replace("\r", "\\r")
        .replace("\n", "\\n")
    )


def write_langpack(code: str, config: dict[str, object]) -> None:
    translations = fetch_translations(int(config["translation_id"]))
    translations.update(CUSTOM_TRANSLATIONS[code])

    if code == "ru":
        translations = OrderedDict(
            (
                source,
                translated
                .replace("\u0401", "\u0415")
                .replace("\u0451", "\u0435"),
            )
            for source, translated in translations.items()
        )

    lines = [
        "Language Pack",
        f"Language: {config['name']}",
        "Last-Modified-Using: 4phone",
        "Authors: MicroSIP community, 4phone",
        "Author-email: support@4phone.uz",
        f"Locale: {config['locale']}",
        "RTL: 0",
        "",
    ]
    for source, translated in translations.items():
        lines.append(f"[{escape_langpack_value(source)}]")
        lines.append(escape_langpack_value(translated))
        lines.append("")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    destination = OUTPUT_DIR / str(config["filename"])
    destination.write_text("\n".join(lines), encoding="utf-8-sig", newline="\n")
    print(f"Wrote {destination.relative_to(ROOT)} with {len(translations)} entries")


def main() -> None:
    for code, config in LANGUAGES.items():
        write_langpack(code, config)


if __name__ == "__main__":
    main()
