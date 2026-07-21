# exif_python.py — просмотр EXIF с картой съемки на Python

import os
import sys
import argparse
import json
import webbrowser
from datetime import datetime
from PIL import Image
import exifread
import folium
from folium.plugins import MarkerCluster

class EXIFViewer:
    def __init__(self):
        self.photos = []
        self.gps_points = []

    def read_exif(self, filepath):
        """Читает EXIF-данные из файла"""
        with open(filepath, 'rb') as f:
            tags = exifread.process_file(f)
        result = {
            'file': os.path.basename(filepath),
            'path': filepath,
            'tags': {}
        }
        # Основные теги
        tag_map = {
            'Image Make': 'камера',
            'Image Model': 'модель',
            'EXIF LensModel': 'объектив',
            'EXIF ExposureTime': 'выдержка',
            'EXIF FNumber': 'диафрагма',
            'EXIF ISOSpeedRatings': 'ISO',
            'EXIF DateTimeOriginal': 'дата',
            'EXIF FocalLength': 'фокусное',
            'EXIF Flash': 'вспышка',
            'EXIF WhiteBalance': 'баланс_белого'
        }
        for exif_tag, readable in tag_map.items():
            if exif_tag in tags:
                result['tags'][readable] = str(tags[exif_tag])

        # GPS координаты
        if 'GPS GPSLatitude' in tags and 'GPS GPSLongitude' in tags:
            lat = self._gps_to_decimal(tags['GPS GPSLatitude'])
            lon = self._gps_to_decimal(tags['GPS GPSLongitude'])
            result['gps'] = (lat, lon)
            self.gps_points.append((lat, lon, result['file']))
        else:
            result['gps'] = None
        return result

    def _gps_to_decimal(self, gps_tag):
        """Преобразует GPS-координаты из формата EXIF в десятичные градусы"""
        values = gps_tag.values
        if len(values) == 3:
            degrees = float(values[0].num) / float(values[0].den)
            minutes = float(values[1].num) / float(values[1].den)
            seconds = float(values[2].num) / float(values[2].den)
            return degrees + minutes/60 + seconds/3600
        return 0.0

    def process_file(self, filepath):
        result = self.read_exif(filepath)
        self.photos.append(result)
        return result

    def process_directory(self, dirpath):
        extensions = ('.jpg', '.jpeg', '.png', '.bmp', '.tiff')
        files = []
        for f in os.listdir(dirpath):
            if f.lower().endswith(extensions):
                files.append(os.path.join(dirpath, f))
        files.sort()
        for f in files:
            try:
                self.process_file(f)
                print(f"Обработано {len(self.photos)}/{len(files)}", end='\r')
            except Exception as e:
                print(f"Ошибка обработки {f}: {e}")
        print()
        print(f"Обработано {len(self.photos)} файлов.")
        print(f"Из них с GPS: {len(self.gps_points)}")

    def generate_map(self, output_file='map.html'):
        if not self.gps_points:
            print("Нет GPS-координат для отображения на карте.")
            return
        # Центр карты
        center = self.gps_points[0][:2]
        m = folium.Map(location=center, zoom_start=12)
        cluster = MarkerCluster().add_to(m)
        for lat, lon, name in self.gps_points:
            folium.Marker(
                location=[lat, lon],
                popup=name,
                icon=folium.Icon(color='blue', icon='camera')
            ).add_to(cluster)
        m.save(output_file)
        print(f"Карта сохранена в {output_file}")
        webbrowser.open(output_file)

    def export_json(self, output_file='exif_data.json'):
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(self.photos, f, indent=2, ensure_ascii=False)
        print(f"Данные сохранены в {output_file}")

    def print_info(self, result=None):
        if result is None:
            if not self.photos:
                print("Нет данных.")
                return
            result = self.photos[0]
        print(f"📷 Файл: {result['file']}")
        tags = result.get('tags', {})
        print(f"Камера: {tags.get('камера', 'неизвестно')} {tags.get('модель', '')}")
        print(f"Объектив: {tags.get('объектив', 'неизвестно')}")
        print(f"Выдержка: {tags.get('выдержка', 'неизвестно')}")
        print(f"Диафрагма: {tags.get('диафрагма', 'неизвестно')}")
        print(f"ISO: {tags.get('ISO', 'неизвестно')}")
        print(f"Дата: {tags.get('дата', 'неизвестно')}")
        if result.get('gps'):
            lat, lon = result['gps']
            print(f"GPS: {lat:.4f}° N, {lon:.4f}° E")
        else:
            print("GPS: отсутствует")

def main():
    parser = argparse.ArgumentParser(description="Просмотр EXIF и карта съемки")
    parser.add_argument('input', help="Файл или папка для обработки")
    parser.add_argument('--output', default='map.html', help="Выходной HTML-файл для карты")
    parser.add_argument('--json', help="Сохранить данные в JSON")
    parser.add_argument('--no-map', action='store_true', help="Не создавать карту")
    args = parser.parse_args()

    viewer = EXIFViewer()
    if os.path.isfile(args.input):
        result = viewer.process_file(args.input)
        viewer.print_info(result)
        if result.get('gps') and not args.no_map:
            viewer.generate_map(args.output)
    elif os.path.isdir(args.input):
        viewer.process_directory(args.input)
        if viewer.photos:
            viewer.print_info(viewer.photos[0])
        if not args.no_map:
            viewer.generate_map(args.output)
        if args.json:
            viewer.export_json(args.json)
    else:
        print("Указанный путь не существует.")
        sys.exit(1)

if __name__ == "__main__":
    main()
