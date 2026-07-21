🗺️ EXIFMapper — просмотр EXIF с картой съемки
Интеллектуальный инструмент для извлечения и визуализации метаданных EXIF из фотографий с отображением места съемки на карте.
Поддерживает все основные теги EXIF: камера, объектив, выдержка, диафрагма, ISO, дата, GPS-координаты и многое другое.
Реализован на 7 языках программирования для демонстрации работы с метаданными и геолокацией.

https://img.shields.io/github/repo-size/yourname/exifmapper
https://img.shields.io/github/stars/yourname/exifmapper?style=social
https://img.shields.io/badge/License-MIT-blue.svg

🧠 Концепция
EXIFMapper — это не просто просмотрщик EXIF. Это полноценный инструмент для анализа и визуализации фотографий:

✅ Извлечение всех EXIF-тегов: камера, объектив, выдержка, диафрагма, ISO, фокусное расстояние, вспышка, баланс белого, дата/время съемки.

✅ GPS-координаты: извлечение широты, долготы и высоты (при наличии).

✅ Отображение на карте: генерация HTML-страницы с картой (Leaflet + OpenStreetMap) для визуализации мест съемки.

✅ Список фотографий: с группировкой по дате или месту.

✅ Экспорт в JSON/CSV: для дальнейшего анализа.

✅ Поддержка пакетного режима: обработка целых папок с фотографиями.

✅ Фильтрация: по дате, камере, наличию GPS и т.д.

✅ Статистика: количество фотографий, диапазон дат, общее количество GPS-точек.

🚀 Как запустить
Для работы с EXIF требуются соответствующие библиотеки. Инструкции по установке и запуску:

Python
bash
pip install exifread pillow folium
python exif_python.py --input photo.jpg
python exif_python.py --input ./photos --output map.html
C++
bash
# Требуется libexif (sudo apt install libexif-dev)
g++ -std=c++17 exif_cpp.cpp -o exif `pkg-config --cflags --libs libexif`
./exif photo.jpg
./exif --dir ./photos --output map.html
Java
bash
# Требуется metadata-extractor (скачать .jar)
javac -cp .:metadata-extractor.jar exif_java.java
java -cp .:metadata-extractor.jar exif_java photo.jpg
java -cp .:metadata-extractor.jar exif_java --dir ./photos --output map.html
C# (.NET Core)
bash
dotnet add package MetadataExtractor
dotnet run -- photo.jpg
dotnet run -- --dir ./photos --output map.html
Go
bash
go get github.com/rwcarlsen/goexif/exif
go run exif_go.go photo.jpg
go run exif_go.go --dir ./photos --output map.html
Rust
bash
cargo add exif
cargo build --release
./target/release/exif_rs photo.jpg
./target/release/exif_rs --dir ./photos --output map.html
JavaScript (Node.js)
bash
npm install exif-parser
node exif_js.js photo.jpg
node exif_js.js --dir ./photos --output map.html
🧩 Пример использования
bash
# Просмотр EXIF одного файла
$ exif photo.jpg
📷 Файл: photo.jpg
Камера: Canon EOS 5D Mark IV
Объектив: EF 24-70mm f/2.8L II USM
Выдержка: 1/125 с
Диафрагма: f/4.0
ISO: 400
Дата: 2024-06-15 14:30:22
GPS: 55.7558° N, 37.6173° E (высота 156 м)

# Создание карты для всех фото в папке
$ exif --dir ./photos --output map.html
Обработано 25 файлов.
Из них с GPS: 18
Карта сохранена в map.html
Открываем карту в браузере...
📦 Содержимое репозитория
Файл	Язык	Особенности
exif_python.py	Python	exifread, folium для карты, цветной вывод, статистика
exif_cpp.cpp	C++	libexif, генерация HTML-карты, пакетный режим
exif_java.java	Java	metadata-extractor, карта через Leaflet, экспорт JSON
exif_cs.cs	C#	MetadataExtractor, асинхронная обработка, карта
exif_go.go	Go	goexif, встроенный HTML-генератор, цветной вывод
exif_rs.rs	Rust	exif crate, быстрый парсинг, карта через OpenStreetMap
exif_js.js	JavaScript	exif-parser, простое CLI, генерация HTML-карты
🔮 Расширенные функции
Группировка по местоположению: кластеризация точек на карте.

Сортировка по дате: автоматическое упорядочивание фотографий.

Экспорт в GPX: для импорта в навигационные приложения.

Интерактивная карта: клик по маркеру показывает миниатюру фото (в планах).

📜 Лицензия
MIT — свободно используйте, модифицируйте и распространяйте.

🤝 Вклад
Приветствуются пул-реквесты с улучшениями, поддержкой новых форматов и расширением функциональности.

⭐ Если проект помогает вам анализировать фотографии — поставьте звёздочку!
