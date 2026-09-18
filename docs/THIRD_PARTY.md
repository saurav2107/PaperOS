# Source provenance

Standalone reference clones have been removed. PaperOS compiles local adapters from `src/apps` and dependencies from `platformio.ini`.

| Reference | Module |
| --- | --- |
| https://github.com/Boisti13/papers3-dashboard | Dashboard/HA reference |
| https://github.com/squirmen/PaperS3Weather | Weather |
| https://github.com/micokonsep/pomodoro-papers3 | Focus |
| https://github.com/juicecultus/EPub-M5Stack-Paper-S3/tree/experimental | Earlier EPUB work |
| https://github.com/juicecultus/crosspoint-reader-papers3 | EPUB design reference; current adapter is self-contained |
| https://github.com/arunmathaisk/PaperS3-chess | Chess layout/assets/initial turn flow |

The removed chess checkout was at `7ce726baecd80d9b030d11ecad7ee816f9716596`. Its twelve PNGs are retained under `assets/chess`. A reference URL does not mean the complete upstream engine is integrated.

## Retained chess asset notice

The chess PNG assets originated in `arunmathaisk/PaperS3-chess`:

> MIT License — Copyright (c) 2026 Arun Mathai
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

PaperOS itself is licensed by the single `LICENSE` file at the repository root. Third-party copyrights remain with their respective owners.
