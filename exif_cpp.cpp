// exif_cpp.cpp — просмотр EXIF с картой съемки на C++ (libexif)

#include <libexif/exif-data.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <cmath>

namespace fs = std::filesystem;

struct PhotoData {
    std::string file;
    std::string make, model, lens;
    std::string exposure, aperture, iso, datetime, focal;
    double gps_lat = 0.0, gps_lon = 0.0;
    bool has_gps = false;
};

class EXIFViewer {
private:
    std::vector<PhotoData> photos;
    std::vector<std::tuple<double, double, std::string>> gps_points;

    double convert_gps(ExifRational* value) {
        if (!value) return 0.0;
        double deg = (double)value[0].numerator / value[0].denominator;
        double min = (double)value[1].numerator / value[1].denominator;
        double sec = (double)value[2].numerator / value[2].denominator;
        return deg + min/60.0 + sec/3600.0;
    }

    std::string rational_to_string(ExifRational* r) {
        if (!r) return "";
        if (r->denominator == 0) return "";
        double val = (double)r->numerator / r->denominator;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << val;
        return ss.str();
    }

public:
    PhotoData process_file(const std::string& path) {
        PhotoData data;
        data.file = fs::path(path).filename().string();
        ExifData* edata = exif_data_new_from_file(path.c_str());
        if (!edata) return data;

        // Получение тегов
        auto get_text = [&](ExifIfd ifd, ExifTag tag) -> std::string {
            ExifEntry* entry = exif_data_get_entry(edata, tag);
            if (!entry) return "";
            char buf[1024];
            exif_entry_get_value(entry, buf, sizeof(buf));
            return std::string(buf);
        };

        data.make = get_text(EXIF_IFD_0, EXIF_TAG_MAKE);
        data.model = get_text(EXIF_IFD_0, EXIF_TAG_MODEL);
        data.lens = get_text(EXIF_IFD_EXIF, EXIF_TAG_LENS_MODEL);
        data.exposure = get_text(EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME);
        data.aperture = get_text(EXIF_IFD_EXIF, EXIF_TAG_FNUMBER);
        data.iso = get_text(EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS);
        data.datetime = get_text(EXIF_IFD_0, EXIF_TAG_DATE_TIME);
        data.focal = get_text(EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH);

        // GPS
        ExifEntry* lat_entry = exif_data_get_entry(edata, EXIF_TAG_GPS_LATITUDE);
        ExifEntry* lon_entry = exif_data_get_entry(edata, EXIF_TAG_GPS_LONGITUDE);
        if (lat_entry && lon_entry) {
            ExifRational* lat_r = (ExifRational*)lat_entry->data;
            ExifRational* lon_r = (ExifRational*)lon_entry->data;
            data.gps_lat = convert_gps(lat_r);
            data.gps_lon = convert_gps(lon_r);
            // Проверка направления
            ExifEntry* lat_ref = exif_data_get_entry(edata, EXIF_TAG_GPS_LATITUDE_REF);
            ExifEntry* lon_ref = exif_data_get_entry(edata, EXIF_TAG_GPS_LONGITUDE_REF);
            if (lat_ref && std::string((char*)lat_ref->data) == "S") data.gps_lat = -data.gps_lat;
            if (lon_ref && std::string((char*)lon_ref->data) == "W") data.gps_lon = -data.gps_lon;
            data.has_gps = true;
            gps_points.push_back({data.gps_lat, data.gps_lon, data.file});
        }
        exif_data_unref(edata);
        return data;
    }

    void process_directory(const std::string& dir) {
        fs::path root(dir);
        std::vector<std::string> files;
        for (auto& entry : fs::directory_iterator(root)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                    files.push_back(entry.path().string());
                }
            }
        }
        std::sort(files.begin(), files.end());
        int total = files.size();
        for (int i = 0; i < total; ++i) {
            auto data = process_file(files[i]);
            photos.push_back(data);
            std::cout << "\rОбработано " << i+1 << "/" << total << std::flush;
        }
        std::cout << std::endl;
        std::cout << "Обработано " << photos.size() << " файлов." << std::endl;
        std::cout << "Из них с GPS: " << gps_points.size() << std::endl;
    }

    void print_info(const PhotoData& data) {
        std::cout << "📷 Файл: " << data.file << std::endl;
        std::cout << "Камера: " << data.make << " " << data.model << std::endl;
        std::cout << "Объектив: " << data.lens << std::endl;
        std::cout << "Выдержка: " << data.exposure << " с" << std::endl;
        std::cout << "Диафрагма: f/" << data.aperture << std::endl;
        std::cout << "ISO: " << data.iso << std::endl;
        std::cout << "Дата: " << data.datetime << std::endl;
        if (data.has_gps) {
            std::cout << "GPS: " << std::fixed << std::setprecision(4) 
                      << data.gps_lat << "° N, " << data.gps_lon << "° E" << std::endl;
        } else {
            std::cout << "GPS: отсутствует" << std::endl;
        }
    }

    void generate_map(const std::string& output) {
        if (gps_points.empty()) {
            std::cout << "Нет GPS-координат для отображения на карте." << std::endl;
            return;
        }
        std::ofstream html(output);
        if (!html.is_open()) return;
        // Центр карты
        double center_lat = std::get<0>(gps_points[0]);
        double center_lon = std::get<1>(gps_points[0]);

        html << "<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>"
             << "<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>"
             << "<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>"
             << "<style>body{margin:0}#map{height:100vh}</style></head><body>"
             << "<div id='map'></div><script>"
             << "var map = L.map('map').setView([" << center_lat << "," << center_lon << "], 12);"
             << "L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);"
             << "var markers = L.markerClusterGroup();";

        for (auto& p : gps_points) {
            double lat = std::get<0>(p);
            double lon = std::get<1>(p);
            std::string name = std::get<2>(p);
            html << "markers.addLayer(L.marker([" << lat << "," << lon << "]).bindPopup('" << name << "'));";
        }
        html << "map.addLayer(markers);"
             << "</script></body></html>";
        html.close();
        std::cout << "Карта сохранена в " << output << std::endl;
        std::cout << "Открываем карту в браузере..." << std::endl;
        std::string cmd = "xdg-open " + output;
        system(cmd.c_str());
    }

    void run(int argc, char** argv) {
        if (argc < 2) {
            std::cout << "Использование: " << argv[0] << " <файл> [--dir <папка>] [--output <map.html>]" << std::endl;
            return;
        }
        std::string input = argv[1];
        std::string output = "map.html";
        bool dir_mode = false;
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--dir") {
                dir_mode = true;
                if (i+1 < argc) input = argv[++i];
            } else if (std::string(argv[i]) == "--output" && i+1 < argc) {
                output = argv[++i];
            }
        }
        if (dir_mode) {
            process_directory(input);
            if (!photos.empty()) print_info(photos[0]);
            generate_map(output);
        } else {
            auto data = process_file(input);
            print_info(data);
            if (data.has_gps) generate_map(output);
        }
    }
};

int main(int argc, char** argv) {
    EXIFViewer viewer;
    viewer.run(argc, argv);
    return 0;
}
