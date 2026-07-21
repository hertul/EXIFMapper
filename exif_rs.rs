// exif_rs.rs — просмотр EXIF с картой съемки на Rust (exif crate)

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::io::BufReader;
use exif::{Reader, Tag, In};
use std::f64;

#[derive(Debug)]
struct PhotoData {
    file: String,
    make: String,
    model: String,
    lens: String,
    exposure: String,
    aperture: String,
    iso: String,
    datetime: String,
    focal: String,
    lat: f64,
    lon: f64,
    has_gps: bool,
}

struct GPSPoint {
    lat: f64,
    lon: f64,
    name: String,
}

fn process_file(path: &Path) -> PhotoData {
    let file = std::fs::File::open(path).unwrap();
    let mut reader = BufReader::new(file);
    let exif = Reader::new().read_from_container(&mut reader).unwrap();

    let mut data = PhotoData {
        file: path.file_name().unwrap().to_string_lossy().to_string(),
        make: String::new(),
        model: String::new(),
        lens: String::new(),
        exposure: String::new(),
        aperture: String::new(),
        iso: String::new(),
        datetime: String::new(),
        focal: String::new(),
        lat: 0.0,
        lon: 0.0,
        has_gps: false,
    };

    for field in exif.fields() {
        let val = field.display_value().to_string();
        match field.tag {
            Tag::Make => data.make = val,
            Tag::Model => data.model = val,
            Tag::LensModel => data.lens = val,
            Tag::ExposureTime => data.exposure = val,
            Tag::FNumber => data.aperture = val,
            Tag::ISOSpeedRatings => data.iso = val,
            Tag::DateTimeOriginal => data.datetime = val,
            Tag::FocalLength => data.focal = val,
            Tag::GPSLatitudeRef => {
                let lat_ref = val;
                if let Some(lat_val) = exif.get_field(Tag::GPSLatitude, In::PRIMARY) {
                    let lat = lat_val.value.get_float(0).unwrap_or(0.0);
                    data.lat = if lat_ref == "S" { -lat } else { lat };
                    data.has_gps = true;
                }
            }
            Tag::GPSLongitudeRef => {
                let lon_ref = val;
                if let Some(lon_val) = exif.get_field(Tag::GPSLongitude, In::PRIMARY) {
                    let lon = lon_val.value.get_float(0).unwrap_or(0.0);
                    data.lon = if lon_ref == "W" { -lon } else { lon };
                    data.has_gps = true;
                }
            }
            _ => {}
        }
    }
    data
}

fn print_info(data: &PhotoData) {
    println!("📷 Файл: {}", data.file);
    println!("Камера: {} {}", data.make, data.model);
    println!("Объектив: {}", data.lens);
    println!("Выдержка: {} с", data.exposure);
    println!("Диафрагма: f/{}", data.aperture);
    println!("ISO: {}", data.iso);
    println!("Дата: {}", data.datetime);
    if data.has_gps {
        println!("GPS: {:.4}° N, {:.4}° E", data.lat, data.lon);
    } else {
        println!("GPS: отсутствует");
    }
}

fn generate_map(gps_points: &[GPSPoint], output: &str) {
    if gps_points.is_empty() {
        println!("Нет GPS-координат для отображения на карте.");
        return;
    }
    let mut html = String::new();
    html.push_str("<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>\n");
    html.push_str("<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>\n");
    html.push_str("<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>\n");
    html.push_str("<style>body{margin:0}#map{height:100vh}</style></head><body>\n");
    html.push_str("<div id='map'></div><script>\n");
    let center = &gps_points[0];
    html.push_str(&format!("var map = L.map('map').setView([{:.4},{:.4}], 12);\n", center.lat, center.lon));
    html.push_str("L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);\n");
    for p in gps_points {
        html.push_str(&format!("L.marker([{:.4},{:.4}]).bindPopup('{}').addTo(map);\n", p.lat, p.lon, p.name));
    }
    html.push_str("</script></body></html>");
    fs::write(output, html).unwrap();
    println!("Карта сохранена в {}", output);
    // открываем в браузере
    let _ = std::process::Command::new("xdg-open").arg(output).spawn();
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        println!("Использование: cargo run -- <файл> --dir <папка> --output <map.html>");
        return;
    }
    let mut input = String::new();
    let mut output = "map.html".to_string();
    let mut dir_mode = false;
    let mut i = 1;
    while i < args.len() {
        if args[i] == "--dir" { dir_mode = true; i += 1; if i < args.len() { input = args[i].clone(); } }
        else if args[i] == "--output" { i += 1; if i < args.len() { output = args[i].clone(); } }
        else if input.is_empty() { input = args[i].clone(); }
        i += 1;
    }
    if input.is_empty() {
        println!("Укажите файл или папку.");
        return;
    }
    let path = Path::new(&input);
    if dir_mode && path.is_dir() {
        let entries = fs::read_dir(path).unwrap();
        let mut files = Vec::new();
        for entry in entries {
            let entry = entry.unwrap();
            let p = entry.path();
            if p.is_file() {
                if let Some(ext) = p.extension() {
                    let ext = ext.to_string_lossy().to_lowercase();
                    if ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" {
                        files.push(p);
                    }
                }
            }
        }
        files.sort();
        let mut photos = Vec::new();
        let mut gps_points = Vec::new();
        let total = files.len();
        for (idx, p) in files.iter().enumerate() {
            let data = process_file(p);
            if data.make != "" || data.model != "" {
                photos.push(data);
                if photos.last().unwrap().has_gps {
                    let d = photos.last().unwrap();
                    gps_points.push(GPSPoint { lat: d.lat, lon: d.lon, name: d.file.clone() });
                }
            }
            println!("\rОбработано {}/{}", idx+1, total);
        }
        println!();
        println!("Обработано {} файлов.", photos.len());
        println!("Из них с GPS: {}", gps_points.len());
        if !photos.is_empty() {
            print_info(&photos[0]);
            generate_map(&gps_points, &output);
        }
    } else {
        let data = process_file(path);
        print_info(&data);
        if data.has_gps {
            let gps = vec![GPSPoint { lat: data.lat, lon: data.lon, name: data.file }];
            generate_map(&gps, &output);
        }
    }
}
