# PaperOS Weather Icons

These PNGs are 96 x 96 rasterisations of the supplied Weather Icons SVG files
from `/Users/sauravsingh/Downloads/weather-icons-master 2/svg`. Their source
names are intentionally preserved.

Copy all eight PNG files to the device SD card at:

```text
/PaperOS/icons/weather/
```

The Weather app maps Open-Meteo codes to these files:

| Condition | Asset |
| --- | --- |
| Clear | `wi-day-sunny.png` |
| Mainly clear / partly cloudy | `wi-day-cloudy.png` |
| Overcast | `wi-cloudy.png` |
| Fog | `wi-fog.png` |
| Drizzle / rain | `wi-rain.png` |
| Snow | `wi-snow.png` |
| Rain showers | `wi-showers.png` |
| Thunderstorm | `wi-thunderstorm.png` |

If an asset is absent, the app automatically uses the built-in fallback icon
instead of leaving the condition blank.
