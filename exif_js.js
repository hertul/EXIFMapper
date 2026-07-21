// exif_js.js — просмотр EXIF с картой съемки на JavaScript (Node.js)

const fs = require('fs');
const path = require('path');
const exifParser = require('exif-parser');

class EXIFViewer {
    constructor() {
        this.photos = [];
        this.gpsPoints = [];
    }

    processFile(filePath) {
        const buffer = fs.readFileSync(filePath);
        const parser = exifParser.create(buffer);
        const result = parser.parse();
        const data = {
            file: path.basename(filePath),
            tags: {}
        };
        // Основные теги
        const tagMap = {
            'Make': 'камера',
            'Model': 'модель',
            'LensModel': 'объектив',
            'ExposureTime': 'выдержка',
            'FNumber': 'диафрагма',
            'ISOSpeedRatings': 'ISO',
            'DateTimeOriginal': 'дата',
            'FocalLength': 'фокусное'
        };
        if (result.tags) {
            for (const [key, val] of Object.entries(result.tags)) {
                if (tagMap[key]) {
                    data.tags[tagMap[key]] = val;
                }
            }
        }
        // GPS
        if (result.tags && result.tags.GPSLatitude && result.tags.GPSLongitude) {
            const lat = result.tags.GPSLatitude;
            const lon = result.tags.GPSLongitude;
            // Преобразование
            const latDeg = lat[0] + lat[1]/60 + lat[2]/3600;
            const lonDeg = lon[0] + lon[1]/60 + lon[2]/3600;
            data.lat = result.tags.GPSLatitudeRef === 'S' ? -latDeg : latDeg;
            data.lon = result.tags.GPSLongitudeRef === 'W' ? -lonDeg : lonDeg;
            data.hasGps = true;
            this.gpsPoints.push({ lat: data.lat, lon: data.lon, name: data.file });
        } else {
            data.hasGps = false;
        }
        return data;
    }

    processDirectory(dirPath) {
        const files = fs.readdirSync(dirPath)
            .filter(f => /\.(jpg|jpeg|png|bmp)$/i.test(f))
            .sort();
        const total = files.length;
        for (let i = 0; i < total; i++) {
            const fullPath = path.join(dirPath, files[i]);
            try {
                const data = this.processFile(fullPath);
                this.photos.push(data);
                process.stdout.write(`\rОбработано ${i+1}/${total}`);
            } catch (e) {
                console.log(`\nОшибка обработки ${files[i]}: ${e.message}`);
            }
        }
        console.log();
        console.log(`Обработано ${this.photos.length} файлов.`);
        console.log(`Из них с GPS: ${this.gpsPoints.length}`);
    }

    printInfo(data) {
        console.log(`📷 Файл: ${data.file}`);
        console.log(`Камера: ${data.tags.камера || 'неизвестно'} ${data.tags.модель || ''}`);
        console.log(`Объектив: ${data.tags.объектив || 'неизвестно'}`);
        console.log(`Выдержка: ${data.tags.выдержка || 'неизвестно'}`);
        console.log(`Диафрагма: ${data.tags.диафрагма || 'неизвестно'}`);
        console.log(`ISO: ${data.tags.ISO || 'неизвестно'}`);
        console.log(`Дата: ${data.tags.дата || 'неизвестно'}`);
        if (data.hasGps) {
            console.log(`GPS: ${data.lat.toFixed(4)}° N, ${data.lon.toFixed(4)}° E`);
        } else {
            console.log('GPS: отсутствует');
        }
    }

    generateMap(output) {
        if (this.gpsPoints.length === 0) {
            console.log('Нет GPS-координат для отображения на карте.');
            return;
        }
        let html = `<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>
<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>
<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>
<style>body{margin:0}#map{height:100vh}</style></head><body>
<div id='map'></div><script>
var map = L.map('map').setView([${this.gpsPoints[0].lat.toFixed(4)}, ${this.gpsPoints[0].lon.toFixed(4)}], 12);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
`;
        for (const p of this.gpsPoints) {
            html += `L.marker([${p.lat.toFixed(4)}, ${p.lon.toFixed(4)}]).bindPopup('${p.name}').addTo(map);\n`;
        }
        html += `</script></body></html>`;
        fs.writeFileSync(output, html);
        console.log(`Карта сохранена в ${output}`);
        // Открываем в браузере (Linux)
        const { exec } = require('child_process');
        exec(`xdg-open ${output}`);
    }

    run(args) {
        if (args.length < 1) {
            console.log('Использование: node exif_js.js <файл> [--dir <папка>] [--output <map.html>]');
            return;
        }
        let input = args[0];
        let output = 'map.html';
        let dirMode = false;
        for (let i = 1; i < args.length; i++) {
            if (args[i] === '--dir' && i+1 < args.length) { dirMode = true; input = args[++i]; }
            else if (args[i] === '--output' && i+1 < args.length) { output = args[++i]; }
        }
        if (dirMode) {
            this.processDirectory(input);
            if (this.photos.length > 0) {
                this.printInfo(this.photos[0]);
            }
            this.generateMap(output);
        } else {
            const data = this.processFile(input);
            this.photos.push(data);
            this.printInfo(data);
            if (data.hasGps) {
                this.generateMap(output);
            }
        }
    }
}

const viewer = new EXIFViewer();
viewer.run(process.argv.slice(2));
