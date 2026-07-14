#!/usr/bin/env python3
"""Generate every 4phone Windows image from the single master logo."""

from __future__ import annotations

import argparse
import hashlib
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / "logo_360x100.png"
EXPECTED_MASTER_SHA256 = (
    "373db2c84bdd6b4fbf71343131c32514bedf3437120eabefc32ac389941712f2"
)

NAVY = (3, 19, 36, 255)
GREEN = (0, 189, 122, 255)
GREEN_DARK = (0, 148, 95, 255)
WHITE = (255, 255, 255, 255)
GREY = (117, 129, 143, 255)
LIGHT_GREY = (226, 232, 240, 255)
YELLOW = (245, 158, 11, 255)
RED = (225, 74, 83, 255)
BLUE = (3, 105, 161, 255)
PURPLE = (124, 58, 237, 255)

ICO_SIZES = [
    (16, 16),
    (20, 20),
    (24, 24),
    (32, 32),
    (40, 40),
    (48, 48),
    (64, 64),
    (128, 128),
    (256, 256),
]


def alpha_bbox(image: Image.Image) -> tuple[int, int, int, int]:
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("master logo has no visible pixels")
    return bbox


def find_mark(master: Image.Image) -> Image.Image:
    alpha = master.getchannel("A")
    columns = [
        x
        for x in range(master.width)
        if alpha.crop((x, 0, x + 1, master.height)).getbbox() is not None
    ]
    if not columns:
        raise ValueError("master logo has no visible columns")

    start = columns[0]
    end = start
    for column in columns[1:]:
        if column > end + 1:
            break
        end = column

    segment = master.crop((start, 0, end + 1, master.height))
    return segment.crop(alpha_bbox(segment))


def contain(
    image: Image.Image,
    size: tuple[int, int],
    padding: int = 0,
) -> Image.Image:
    available = (size[0] - padding * 2, size[1] - padding * 2)
    scale = min(available[0] / image.width, available[1] / image.height)
    resized = image.resize(
        (
            max(1, round(image.width * scale)),
            max(1, round(image.height * scale)),
        ),
        Image.Resampling.LANCZOS,
    )
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(
        resized,
        ((size[0] - resized.width) // 2, (size[1] - resized.height) // 2),
    )
    return canvas


def app_icon(mark: Image.Image) -> Image.Image:
    size = 256
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    draw.rounded_rectangle(
        (8, 8, size - 8, size - 8),
        radius=54,
        fill=NAVY,
    )
    fitted = contain(mark, (174, 190), padding=3)
    canvas.alpha_composite(fitted, (41, 33))
    return canvas


def draw_lock(draw: ImageDraw.ImageDraw, color: tuple[int, ...]) -> None:
    draw.rounded_rectangle((162, 151, 225, 220), radius=12, fill=NAVY)
    draw.rounded_rectangle((170, 160, 217, 212), radius=8, fill=color)
    draw.arc((176, 126, 211, 176), 180, 360, fill=NAVY, width=12)
    draw.ellipse((189, 180, 198, 189), fill=NAVY)


def app_status_icon(
    mark: Image.Image,
    status: tuple[int, ...],
    *,
    secure: bool = False,
    missed: bool = False,
) -> Image.Image:
    canvas = app_icon(mark)
    draw = ImageDraw.Draw(canvas)
    draw.ellipse((166, 166, 238, 238), fill=WHITE)
    draw.ellipse((174, 174, 230, 230), fill=status)
    if missed:
        draw.line((202, 185, 202, 211), fill=WHITE, width=9)
        draw.ellipse((197, 218, 207, 228), fill=WHITE)
    if secure:
        draw_lock(draw, status)
    return canvas


def presence_icon(
    color: tuple[int, ...],
    *,
    starred: bool = False,
    phone: bool = False,
    secure: bool = False,
    forwarding: bool = False,
) -> Image.Image:
    canvas = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    draw.ellipse((30, 30, 226, 226), fill=WHITE, outline=LIGHT_GREY, width=14)
    draw.ellipse((57, 57, 199, 199), fill=color)

    if phone:
        draw.arc((76, 70, 180, 184), 130, 315, fill=WHITE, width=22)
        draw.ellipse((68, 145, 99, 178), fill=WHITE)
        draw.ellipse((157, 74, 188, 107), fill=WHITE)
    elif forwarding:
        draw.line((83, 129, 170, 129), fill=WHITE, width=20)
        draw.polygon(((148, 92), (198, 129), (148, 166)), fill=WHITE)

    if secure:
        draw_lock(draw, GREEN)
    if starred:
        points = []
        for index in range(10):
            radius = 41 if index % 2 == 0 else 18
            angle = -90 + index * 36
            import math

            points.append(
                (
                    199 + int(math.cos(math.radians(angle)) * radius),
                    61 + int(math.sin(math.radians(angle)) * radius),
                )
            )
        draw.polygon(points, fill=YELLOW, outline=WHITE)
    return canvas


def phone_glyph(
    color: tuple[int, ...] = NAVY,
    *,
    direction: str | None = None,
    missed: bool = False,
) -> Image.Image:
    canvas = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    draw.arc((42, 34, 210, 218), 130, 312, fill=color, width=34)
    draw.ellipse((31, 151, 78, 202), fill=color)
    draw.ellipse((179, 48, 226, 99), fill=color)

    if direction:
        arrow_color = RED if missed else GREEN
        if direction == "in":
            draw.line((164, 88, 112, 140), fill=arrow_color, width=18)
            draw.polygon(((105, 112), (105, 147), (140, 147)), fill=arrow_color)
        else:
            draw.line((106, 146, 161, 91), fill=arrow_color, width=18)
            draw.polygon(((132, 84), (168, 84), (168, 120)), fill=arrow_color)
    if missed:
        draw.line((105, 89, 171, 155), fill=RED, width=18)
        draw.line((171, 89, 105, 155), fill=RED, width=18)
    return canvas


def action_icon(kind: str, color: tuple[int, ...] = NAVY) -> Image.Image:
    canvas = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    width = 22

    if kind == "close":
        draw.line((70, 70, 186, 186), fill=color, width=width)
        draw.line((186, 70, 70, 186), fill=color, width=width)
    elif kind == "dropdown":
        draw.polygon(((60, 90), (196, 90), (128, 170)), fill=color)
    elif kind == "search":
        draw.ellipse((49, 44, 159, 154), outline=color, width=width)
        draw.line((148, 143, 211, 206), fill=color, width=width)
    elif kind == "message":
        draw.rounded_rectangle((38, 52, 218, 180), radius=24, outline=color, width=width)
        draw.polygon(((85, 174), (85, 220), (133, 178)), fill=color)
    elif kind == "hold":
        draw.rounded_rectangle((66, 45, 105, 211), radius=8, fill=color)
        draw.rounded_rectangle((151, 45, 190, 211), radius=8, fill=color)
    elif kind == "resume":
        draw.polygon(((80, 45), (80, 211), (205, 128)), fill=color)
    elif kind in {"mutein", "mutedout"}:
        draw.polygon(((46, 101), (87, 101), (130, 64), (130, 192), (87, 155), (46, 155)), fill=color)
        draw.line((150, 83, 213, 174), fill=RED, width=width)
        draw.line((213, 83, 150, 174), fill=RED, width=width)
    elif kind in {"muteout", "speaker"}:
        draw.polygon(((40, 101), (83, 101), (130, 63), (130, 193), (83, 155), (40, 155)), fill=color)
        draw.arc((119, 82, 192, 174), 285, 75, fill=color, width=width)
    elif kind == "transfer" or kind == "forward":
        draw.line((48, 128, 180, 128), fill=color, width=width)
        draw.polygon(((156, 78), (220, 128), (156, 178)), fill=color)
    elif kind == "contact":
        draw.ellipse((83, 38, 173, 128), fill=color)
        draw.rounded_rectangle((48, 137, 208, 224), radius=44, fill=color)
    elif kind == "conference":
        draw.ellipse((91, 35, 165, 109), fill=color)
        draw.ellipse((39, 70, 101, 132), fill=GREY)
        draw.ellipse((155, 70, 217, 132), fill=GREY)
        draw.rounded_rectangle((76, 119, 180, 221), radius=45, fill=color)
        draw.rounded_rectangle((24, 137, 93, 211), radius=32, fill=GREY)
        draw.rounded_rectangle((163, 137, 232, 211), radius=32, fill=GREY)
    elif kind == "video":
        draw.rounded_rectangle((35, 65, 164, 191), radius=20, fill=color)
        draw.polygon(((164, 104), (222, 72), (222, 184), (164, 152)), fill=color)
    elif kind == "exit":
        draw.arc((35, 30, 221, 222), 45, 315, fill=color, width=width)
        draw.line((128, 28, 128, 133), fill=RED, width=width)
    else:
        draw.ellipse((55, 55, 201, 201), outline=color, width=width)
    return canvas


def line_icon(kind: str, secure: bool = False) -> Image.Image:
    if kind == "message":
        image = action_icon("message", BLUE)
    elif kind == "conference":
        image = action_icon("conference", PURPLE)
    else:
        image = action_icon("hold", YELLOW)
    if secure:
        draw_lock(ImageDraw.Draw(image), GREEN)
    return image


def recolor_wordmark(master: Image.Image) -> Image.Image:
    image = master.crop(alpha_bbox(master)).copy()
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                continue
            if green > red + 40 and green > blue + 30:
                pixels[x, y] = (GREEN[0], GREEN[1], GREEN[2], alpha)
            else:
                pixels[x, y] = (255, 255, 255, alpha)
    return image


def wizard_images(master: Image.Image, mark: Image.Image) -> dict[str, Image.Image]:
    large = Image.new("RGB", (164, 314), NAVY[:3])
    overlay = Image.new("RGBA", large.size, (0, 0, 0, 0))
    logo = contain(recolor_wordmark(master), (140, 70), padding=2)
    overlay.alpha_composite(logo, (12, 24))
    draw = ImageDraw.Draw(overlay)
    draw.ellipse((82, 178, 238, 334), outline=GREEN, width=18)
    draw.ellipse((108, 204, 212, 308), outline=(255, 255, 255, 70), width=3)
    large.paste(overlay, mask=overlay.getchannel("A"))

    small = Image.new("RGB", (55, 58), (248, 250, 252))
    small_overlay = contain(app_icon(mark), (48, 48), padding=1)
    small.paste(small_overlay, (3, 5), small_overlay)
    return {
        "installer/4phone-wizard.bmp": large,
        "installer/4phone-wizard-small.bmp": small,
    }


def build_assets(master: Image.Image) -> dict[str, Image.Image]:
    mark = find_mark(master)
    assets: dict[str, Image.Image] = {
        "microsip/res/4phone.ico": app_icon(mark),
        "microsip/res/tray_inactive.ico": app_status_icon(mark, GREY),
        "microsip/res/tray_missed.ico": app_status_icon(mark, RED, missed=True),
        "microsip/res/exit.ico": action_icon("exit"),
        "microsip/res/close.ico": action_icon("close"),
        "microsip/res/close2.ico": action_icon("close", WHITE),
        "microsip/res/contact.ico": action_icon("contact"),
        "microsip/res/dropdown.ico": action_icon("dropdown"),
        "microsip/res/messagedpi.ico": action_icon("message"),
        "microsip/res/call_out.ico": phone_glyph(direction="out"),
        "microsip/res/call_in.ico": phone_glyph(direction="in"),
        "microsip/res/call_miss.ico": phone_glyph(missed=True),
        "microsip/res/call_miss1.ico": phone_glyph(RED, missed=True),
        "microsip/res/line_conference.ico": line_icon("conference"),
        "microsip/res/line_conference-secure.ico": line_icon("conference", True),
        "microsip/res/line_message-in.ico": line_icon("message"),
        "microsip/res/line_on-hold.ico": line_icon("hold"),
        "microsip/res/line_on-remote-hold.ico": line_icon("hold"),
        "microsip/res/line_on-remote-hold-conference.ico": line_icon("conference"),
        "microsip/res/status_blank.ico": Image.new("RGBA", (256, 256), (0, 0, 0, 0)),
        "microsip/res/status_default.ico": presence_icon(BLUE),
        "microsip/res/status_default_starred.ico": presence_icon(BLUE, starred=True),
        "microsip/res/status_online.ico": presence_icon(GREEN),
        "microsip/res/status_online_starred.ico": presence_icon(GREEN, starred=True),
        "microsip/res/status_offline.ico": presence_icon(GREY),
        "microsip/res/status_offline_starred.ico": presence_icon(GREY, starred=True),
        "microsip/res/status_away.ico": presence_icon(YELLOW),
        "microsip/res/status_away_starred.ico": presence_icon(YELLOW, starred=True),
        "microsip/res/status_busy.ico": presence_icon(RED),
        "microsip/res/status_busy_starred.ico": presence_icon(RED, starred=True),
        "microsip/res/status_on-the-phone.ico": presence_icon(PURPLE, phone=True),
        "microsip/res/status_on-the-phone_starred.ico": presence_icon(PURPLE, phone=True, starred=True),
        "microsip/res/status_forwarding.ico": presence_icon(BLUE, forwarding=True),
        "microsip/res/status_secure.ico": presence_icon(GREEN, secure=True),
        "microsip/res/status_unknown.ico": presence_icon(LIGHT_GREY),
        "microsip/res/status_unknown_starred.ico": presence_icon(LIGHT_GREY, starred=True),
        "microsip/res/status_yellow.ico": presence_icon(YELLOW),
    }

    active_colors = {
        "green": GREEN,
        "grey": GREY,
        "yellow": YELLOW,
        "red": RED,
        "blue": BLUE,
    }
    for name, color in active_colors.items():
        assets[f"microsip/res/active_{name}.ico"] = app_status_icon(mark, color)
        if name != "blue":
            assets[f"microsip/res/active_{name}_secure.ico"] = app_status_icon(
                mark,
                color,
                secure=True,
            )

    action_files = {
        "button_forward.ico": "forward",
        "button_hold.ico": "hold",
        "button_message.ico": "message",
        "button_mutedin.ico": "mutein",
        "button_mutedout.ico": "mutedout",
        "button_mutein.ico": "mutein",
        "button_muteout.ico": "muteout",
        "button_resume.ico": "resume",
        "button_search.ico": "search",
        "button_speaker.ico": "speaker",
        "button_transfer.ico": "transfer",
        "button_video.ico": "video",
    }
    for filename, kind in action_files.items():
        assets[f"microsip/res/{filename}"] = action_icon(kind)

    assets.update(wizard_images(master, mark))
    return assets


def validate_assets(assets: dict[str, Image.Image]) -> None:
    expected_wizard_sizes = {
        "installer/4phone-wizard.bmp": (164, 314),
        "installer/4phone-wizard-small.bmp": (55, 58),
    }
    for relative, image in assets.items():
        suffix = Path(relative).suffix.lower()
        if suffix == ".ico":
            if image.mode != "RGBA" or image.size != (256, 256):
                raise ValueError(
                    f"{relative} must be a 256x256 RGBA icon source"
                )
            alpha = image.getchannel("A")
            if relative.endswith("status_blank.ico"):
                if alpha.getbbox() is not None:
                    raise ValueError(f"{relative} must be fully transparent")
            else:
                extrema = alpha.getextrema()
                if extrema != (0, 255):
                    raise ValueError(
                        f"{relative} must contain transparent and opaque pixels"
                    )
        elif suffix == ".bmp":
            if image.mode != "RGB":
                raise ValueError(f"{relative} must use RGB mode")
            if image.size != expected_wizard_sizes[relative]:
                raise ValueError(
                    f"{relative} has unexpected dimensions {image.size}"
                )


def save_asset(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.suffix.lower() == ".ico":
        image.save(path, format="ICO", sizes=ICO_SIZES)
    elif path.suffix.lower() == ".bmp":
        image.convert("RGB").save(path, format="BMP")
    else:
        image.save(path)


def generate(output_root: Path) -> list[Path]:
    digest = hashlib.sha256(MASTER.read_bytes()).hexdigest()
    if digest != EXPECTED_MASTER_SHA256:
        raise RuntimeError(
            f"unexpected master logo SHA-256: {digest}"
        )
    master = Image.open(MASTER).convert("RGBA")
    if master.size != (360, 100):
        raise RuntimeError(f"unexpected master logo dimensions: {master.size}")
    if master.getchannel("A").getextrema() != (0, 255):
        raise RuntimeError("master logo must contain transparent and opaque pixels")
    assets = build_assets(master)
    validate_assets(assets)
    generated: list[Path] = []
    for relative, image in sorted(assets.items()):
        destination = output_root / relative
        save_asset(image, destination)
        generated.append(Path(relative))
    return generated


def check() -> int:
    with tempfile.TemporaryDirectory(prefix="4phone-brand-") as directory:
        temporary_root = Path(directory)
        generated = generate(temporary_root)
        mismatches = [
            relative
            for relative in generated
            if not (ROOT / relative).is_file()
            or (ROOT / relative).read_bytes()
            != (temporary_root / relative).read_bytes()
        ]
    if mismatches:
        print("Brand assets are not reproducible or are stale:")
        for mismatch in mismatches:
            print(f"  {mismatch}")
        return 1
    print(f"Validated {len(generated)} reproducible 4phone brand assets")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="compare generated bytes with committed assets",
    )
    args = parser.parse_args()

    if args.check:
        return check()

    generated = generate(ROOT)
    print(f"Generated {len(generated)} 4phone brand assets")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
