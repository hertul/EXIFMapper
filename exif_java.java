// exif_java.java — просмотр EXIF с картой съемки на Java (metadata-extractor)

import com.drew.imaging.ImageMetadataReader;
import com.drew.metadata.Metadata;
import com.drew.metadata.exif.ExifSubIFDDirectory;
import com.drew.metadata.exif.GpsDirectory;
import com.drew.metadata.Tag;
import com.drew.metadata.Directory;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.*;
import java.util.*;

public class EXIFViewer {
    private static class PhotoData {
        String file;
        Map<String, String> tags = new HashMap<>();
        Double lat = null, lon = null;
        boolean hasGps = false;
    }

    private List<PhotoData> photos = new ArrayList<>();
    private List<Triple> gpsPoints = new ArrayList<>();

    private static class Triple {
        double lat, lon;
        String name;
        Triple(double lat, double lon, String name) { this.lat=lat; this.lon=lon; this.name=name; }
    }

    public PhotoData processFile(String path) throws Exception {
        PhotoData data = new PhotoData();
        data.file = Paths.get(path).getFileName().toString();
        File file = new File(path);
        Metadata metadata = ImageMetadataReader.readMetadata(file);

        // Извлечение EXIF
        ExifSubIFDDirectory exifDir = metadata.getFirstDirectoryOfType(ExifSubIFDDirectory.class);
        if (exifDir != null) {
            for (Tag tag : exifDir.getTags()) {
                data.tags.put(tag.getTagName(), tag.getDescription());
            }
        }

        // GPS
        GpsDirectory gpsDir = metadata.getFirstDirectoryOfType(GpsDirectory.class);
        if (gpsDir != null && gpsDir.getGeoLocation() != null) {
            data.lat = gpsDir.getGeoLocation().getLatitude();
            data.lon = gpsDir.getGeoLocation().getLongitude();
            data.hasGps = true;
            gpsPoints.add(new Triple(data.lat, data.lon, data.file));
        }

        // Другие директории
        for (Directory dir : metadata.getDirectories()) {
            if (dir instanceof ExifSubIFDDirectory || dir instanceof GpsDirectory) continue;
            for (Tag tag : dir.getTags()) {
                data.tags.put(tag.getTagName(), tag.getDescription());
            }
        }
        return data;
    }

    public void processDirectory(String dirPath) throws Exception {
        Path dir = Paths.get(dirPath);
        List<String> files = new ArrayList<>();
        try (DirectoryStream<Path> stream = Files.newDirectoryStream(dir)) {
            for (Path entry : stream) {
                String name = entry.getFileName().toString().toLowerCase();
                if (name.endsWith(".jpg") || name.endsWith(".jpeg") || name.endsWith(".png") || name.endsWith(".bmp")) {
                    files.add(entry.toString());
                }
            }
        }
        Collections.sort(files);
        int total = files.size();
        for (int i = 0; i < total; i++) {
            try {
                PhotoData data = processFile(files.get(i));
                photos.add(data);
                System.out.printf("\rОбработано %d/%d", i+1, total);
            } catch (Exception e) {
                System.err.println("Ошибка обработки " + files.get(i) + ": " + e.getMessage());
            }
        }
        System.out.println();
        System.out.println("Обработано " + photos.size() + " файлов.");
        System.out.println("Из них с GPS: " + gpsPoints.size());
    }

    public void printInfo(PhotoData data) {
        System.out.println("📷 Файл: " + data.file);
        System.out.println("Камера: " + data.tags.getOrDefault("Make", "неизвестно") + " " + data.tags.getOrDefault("Model", ""));
        System.out.println("Объектив: " + data.tags.getOrDefault("Lens Model", "неизвестно"));
        System.out.println("Выдержка: " + data.tags.getOrDefault("Exposure Time", "неизвестно"));
        System.out.println("Диафрагма: " + data.tags.getOrDefault("F-Number", "неизвестно"));
        System.out.println("ISO: " + data.tags.getOrDefault("ISO", "неизвестно"));
        System.out.println("Дата: " + data.tags.getOrDefault("Date/Time Original", "неизвестно"));
        if (data.hasGps) {
            System.out.printf("GPS: %.4f° N, %.4f° E\n", data.lat, data.lon);
        } else {
            System.out.println("GPS: отсутствует");
        }
    }

    public void generateMap(String output) throws IOException {
        if (gpsPoints.isEmpty()) {
            System.out.println("Нет GPS-координат для отображения на карте.");
            return;
        }
        try (FileWriter fw = new FileWriter(output)) {
            double centerLat = gpsPoints.get(0).lat;
            double centerLon = gpsPoints.get(0).lon;
            fw.write("<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>\n");
            fw.write("<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>\n");
            fw.write("<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>\n");
            fw.write("<style>body{margin:0}#map{height:100vh}</style></head><body>\n");
            fw.write("<div id='map'></div><script>\n");
            fw.write("var map = L.map('map').setView([" + centerLat + "," + centerLon + "], 12);\n");
            fw.write("L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);\n");
            for (Triple t : gpsPoints) {
                fw.write("L.marker([" + t.lat + "," + t.lon + "]).bindPopup('" + t.name + "').addTo(map);\n");
            }
            fw.write("</script></body></html>");
        }
        System.out.println("Карта сохранена в " + output);
        String cmd = "xdg-open " + output;
        Runtime.getRuntime().exec(cmd);
    }

    public static void main(String[] args) {
        try {
            EXIFViewer viewer = new EXIFViewer();
            if (args.length < 1) {
                System.out.println("Использование: java EXIFViewer <файл> [--dir <папка>] [--output <map.html>]");
                return;
            }
            String input = args[0];
            String output = "map.html";
            boolean dirMode = false;
            for (int i = 1; i < args.length; i++) {
                if (args[i].equals("--dir") && i+1 < args.length) {
                    dirMode = true;
                    input = args[++i];
                } else if (args[i].equals("--output") && i+1 < args.length) {
                    output = args[++i];
                }
            }
            if (dirMode) {
                viewer.processDirectory(input);
                if (!viewer.photos.isEmpty()) viewer.printInfo(viewer.photos.get(0));
                viewer.generateMap(output);
            } else {
                PhotoData data = viewer.processFile(input);
                viewer.printInfo(data);
                if (data.hasGps) viewer.generateMap(output);
            }
        } catch (Exception e) {
            System.err.println("Ошибка: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
