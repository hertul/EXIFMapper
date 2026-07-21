// exif_cs.cs — просмотр EXIF с картой съемки на C# (MetadataExtractor)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Text.Json;
using MetadataExtractor;
using MetadataExtractor.Formats.Exif;
using MetadataExtractor.Formats.Gps;

class EXIFViewer
{
    private class PhotoData
    {
        public string File { get; set; }
        public Dictionary<string, string> Tags { get; set; } = new();
        public double? Lat { get; set; }
        public double? Lon { get; set; }
        public bool HasGps => Lat.HasValue && Lon.HasValue;
    }

    private List<PhotoData> photos = new();
    private List<(double lat, double lon, string name)> gpsPoints = new();

    public PhotoData ProcessFile(string path)
    {
        var data = new PhotoData();
        data.File = Path.GetFileName(path);
        var directories = ImageMetadataReader.ReadMetadata(path);

        foreach (var dir in directories)
        {
            foreach (var tag in dir.Tags)
            {
                data.Tags[tag.Name] = tag.Description;
            }
            if (dir is GpsDirectory gpsDir)
            {
                var location = gpsDir.GetGeoLocation();
                if (location != null)
                {
                    data.Lat = location.Latitude;
                    data.Lon = location.Longitude;
                    gpsPoints.Add((data.Lat.Value, data.Lon.Value, data.File));
                }
            }
        }
        return data;
    }

    public void ProcessDirectory(string dirPath)
    {
        var files = Directory.GetFiles(dirPath)
            .Where(f => f.EndsWith(".jpg", StringComparison.OrdinalIgnoreCase)
                     || f.EndsWith(".jpeg", StringComparison.OrdinalIgnoreCase)
                     || f.EndsWith(".png", StringComparison.OrdinalIgnoreCase)
                     || f.EndsWith(".bmp", StringComparison.OrdinalIgnoreCase))
            .OrderBy(f => f)
            .ToList();

        int total = files.Count;
        for (int i = 0; i < total; i++)
        {
            try
            {
                var data = ProcessFile(files[i]);
                photos.Add(data);
                Console.Write($"\rОбработано {i+1}/{total}");
            }
            catch (Exception e)
            {
                Console.WriteLine($"\nОшибка обработки {files[i]}: {e.Message}");
            }
        }
        Console.WriteLine();
        Console.WriteLine($"Обработано {photos.Count} файлов.");
        Console.WriteLine($"Из них с GPS: {gpsPoints.Count}");
    }

    public void PrintInfo(PhotoData data)
    {
        Console.WriteLine($"📷 Файл: {data.File}");
        Console.WriteLine($"Камера: {data.Tags.GetValueOrDefault("Make", "неизвестно")} {data.Tags.GetValueOrDefault("Model", "")}");
        Console.WriteLine($"Объектив: {data.Tags.GetValueOrDefault("Lens Model", "неизвестно")}");
        Console.WriteLine($"Выдержка: {data.Tags.GetValueOrDefault("Exposure Time", "неизвестно")}");
        Console.WriteLine($"Диафрагма: {data.Tags.GetValueOrDefault("F-Number", "неизвестно")}");
        Console.WriteLine($"ISO: {data.Tags.GetValueOrDefault("ISO", "неизвестно")}");
        Console.WriteLine($"Дата: {data.Tags.GetValueOrDefault("Date/Time Original", "неизвестно")}");
        if (data.HasGps)
            Console.WriteLine($"GPS: {data.Lat:F4}° N, {data.Lon:F4}° E");
        else
            Console.WriteLine("GPS: отсутствует");
    }

    public void GenerateMap(string output)
    {
        if (gpsPoints.Count == 0)
        {
            Console.WriteLine("Нет GPS-координат для отображения на карте.");
            return;
        }
        using var writer = new StreamWriter(output);
        double centerLat = gpsPoints[0].lat;
        double centerLon = gpsPoints[0].lon;
        writer.WriteLine("<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>");
        writer.WriteLine("<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>");
        writer.WriteLine("<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>");
        writer.WriteLine("<style>body{margin:0}#map{height:100vh}</style></head><body>");
        writer.WriteLine($"<div id='map'></div><script>");
        writer.WriteLine($"var map = L.map('map').setView([{centerLat},{centerLon}], 12);");
        writer.WriteLine("L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);");
        foreach (var p in gpsPoints)
        {
            writer.WriteLine($"L.marker([{p.lat},{p.lon}]).bindPopup('{p.name}').addTo(map);");
        }
        writer.WriteLine("</script></body></html>");
        Console.WriteLine($"Карта сохранена в {output}");
        System.Diagnostics.Process.Start("xdg-open", output);
    }

    public static async Task Main(string[] args)
    {
        var viewer = new EXIFViewer();
        if (args.Length < 1)
        {
            Console.WriteLine("Использование: dotnet run <файл> [--dir <папка>] [--output <map.html>]");
            return;
        }
        string input = args[0];
        string output = "map.html";
        bool dirMode = false;
        for (int i = 1; i < args.Length; i++)
        {
            if (args[i] == "--dir" && i+1 < args.Length) { dirMode = true; input = args[++i]; }
            else if (args[i] == "--output" && i+1 < args.Length) { output = args[++i]; }
        }
        if (dirMode)
        {
            viewer.ProcessDirectory(input);
            if (viewer.photos.Count > 0) viewer.PrintInfo(viewer.photos[0]);
            viewer.GenerateMap(output);
        }
        else
        {
            var data = viewer.ProcessFile(input);
            viewer.PrintInfo(data);
            if (data.HasGps) viewer.GenerateMap(output);
        }
    }
}
