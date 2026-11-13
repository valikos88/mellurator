# Mellurator

Сатуратор с мягким бас-бустом. VST3 для FL Studio (и любого другого хоста). Лицо вместо ручки Drive.

## Сборка (Windows, Visual Studio 2022 или Ninja)

```
git clone https://github.com/juce-framework/JUCE.git        # рядом с папкой Mellurator
cd Mellurator
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

С `COPY_PLUGIN_AFTER_BUILD TRUE` готовый `Mellurator.vst3` копируется в `C:\Program Files\Common Files\VST3`.
В FL Studio: Options > Manage plugins > Find installed plugins.

Код не компилировался в среде, где он писался. Ждите мелких правок под вашу версию JUCE (писано под 7/8).

## Сигнал

```
in -> low shelf (90 Hz, 0..+9 dB) -> 4x oversampling
   -> tanh(x * gain + bias) - tanh(bias), gain 0..+24 dB, компенсация до -8 dB
   -> down -> DC-фильтр 18 Hz -> out
```

Bias 0.12 даёт чуть чётных гармоник (тепло). Бас-буст стоит до сатуратора, поэтому поднятые низы тоже получают гармоники.

## Интерфейс

- Две ручки: DRIVE (лицо) и BASS (маленькая).
- `MelluContent` рисует интерфейс как обычно. `PixelOverlay` лежит поверх, не ловит мышь и рисует усреднённую по блокам копию снимка `MelluContent`. Поэтому пикселизируется всё: ручки, текст, фон.
- Размер блока: `1 + 15 * ((drive - 0.06) / 0.94)^1.6`. До drive ~6% оверлей ничего не рисует.
- Покраснение: `mello::tint()` смещает акцент из зелёного (25,224,79) в красный (255,42,26), красит кожу и накладывает красную вуаль.
- После 45% drive появляются глитч-полосы: сдвинутые срезы и яркие блоки. После 30% лицо дрожит.
- Лицо крутится на ±50° (`kFaceTilt` в `PluginEditor.cpp`).

## Шрифты

Сейчас системные. Чтобы получить Russo One как в прототипе: положите TTF в `Assets/`, добавьте в `juce_add_binary_data` и создайте шрифт через `juce::Typeface::createSystemTypefaceFor` в `mello::display()`.
