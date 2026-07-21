// exif_go.go — просмотр EXIF с картой съемки на Go (goexif)

package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"github.com/rwcarlsen/goexif/exif"
)

type PhotoData struct {
	File    string
	Make    string
	Model   string
	Lens    string
	Exposure string
	Aperture string
	ISO      string
	DateTime string
	Focal    string
	Lat      float64
	Lon      float64
	HasGPS   bool
}

type GPSPoint struct {
	Lat  float64
	Lon  float64
	Name string
}

var photos []PhotoData
var gpsPoints []GPSPoint

func processFile(path string) PhotoData {
	data := PhotoData{File: filepath.Base(path)}
	f, err := os.Open(path)
	if err != nil {
		return data
	}
	defer f.Close()
	x, err := exif.Decode(f)
	if err != nil {
		return data
	}
	// Извлечение тегов
	getStr := func(tag exif.FieldName) string {
		if val, err := x.Get(tag); err == nil {
			if str, err := val.StringVal(); err == nil {
				return str
			}
		}
		return ""
	}
	data.Make = getStr(exif.Make)
	data.Model = getStr(exif.Model)
	data.Lens = getStr(exif.LensModel)
	data.Exposure = getStr(exif.ExposureTime)
	data.Aperture = getStr(exif.FNumber)
	data.ISO = getStr(exif.ISOSpeedRatings)
	data.DateTime = getStr(exif.DateTimeOriginal)
	data.Focal = getStr(exif.FocalLength)

	// GPS
	if lat, lon, err := x.LatLong(); err == nil {
		data.Lat = lat
		data.Lon = lon
		data.HasGPS = true
		gpsPoints = append(gpsPoints, GPSPoint{lat, lon, data.File})
	}
	return data
}

func processDirectory(dir string) {
	files, err := ioutil.ReadDir(dir)
	if err != nil {
		fmt.Println("Ошибка чтения каталога:", err)
		return
	}
	var paths []string
	for _, f := range files {
		if !f.IsDir() {
			ext := strings.ToLower(filepath.Ext(f.Name()))
			if ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" {
				paths = append(paths, filepath.Join(dir, f.Name()))
			}
		}
	}
	sort.Strings(paths)
	total := len(paths)
	for i, p := range paths {
		data := processFile(p)
		if data.Make != "" || data.Model != "" {
			photos = append(photos, data)
		}
		fmt.Printf("\rОбработано %d/%d", i+1, total)
	}
	fmt.Println()
	fmt.Printf("Обработано %d файлов.\n", len(photos))
	fmt.Printf("Из них с GPS: %d\n", len(gpsPoints))
}

func printInfo(data PhotoData) {
	fmt.Printf("📷 Файл: %s\n", data.File)
	fmt.Printf("Камера: %s %s\n", data.Make, data.Model)
	fmt.Printf("Объектив: %s\n", data.Lens)
	fmt.Printf("Выдержка: %s с\n", data.Exposure)
	fmt.Printf("Диафрагма: f/%s\n", data.Aperture)
	fmt.Printf("ISO: %s\n", data.ISO)
	fmt.Printf("Дата: %s\n", data.DateTime)
	if data.HasGPS {
		fmt.Printf("GPS: %.4f° N, %.4f° E\n", data.Lat, data.Lon)
	} else {
		fmt.Println("GPS: отсутствует")
	}
}

func generateMap(output string) {
	if len(gpsPoints) == 0 {
		fmt.Println("Нет GPS-координат для отображения на карте.")
		return
	}
	html := `<!DOCTYPE html><html><head><meta charset='utf-8'/><title>EXIF Карта съемки</title>
<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>
<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>
<style>body{margin:0}#map{height:100vh}</style></head><body>
<div id='map'></div><script>
var map = L.map('map').setView([` + fmt.Sprintf("%.4f", gpsPoints[0].Lat) + `,` + fmt.Sprintf("%.4f", gpsPoints[0].Lon) + `], 12);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
`
	for _, p := range gpsPoints {
		html += fmt.Sprintf("L.marker([%.4f,%.4f]).bindPopup('%s').addTo(map);\n", p.Lat, p.Lon, p.Name)
	}
	html += `</script></body></html>`
	err := ioutil.WriteFile(output, []byte(html), 0644)
	if err != nil {
		fmt.Println("Ошибка сохранения карты:", err)
		return
	}
	fmt.Println("Карта сохранена в", output)
	fmt.Println("Открываем карту в браузере...")
	// для Linux
	_ = os.Remove("open.sh")
	_ = os.WriteFile("open.sh", []byte("#!/bin/sh\nxdg-open "+output), 0755)
	_ = os.Chmod("open.sh", 0755)
}

func main() {
	inputPtr := flag.String("input", "", "Файл или папка для обработки")
	outputPtr := flag.String("output", "map.html", "Выходной HTML-файл")
	dirMode := flag.Bool("dir", false, "Режим обработки папки")
	flag.Parse()

	if *inputPtr == "" {
		fmt.Println("Использование: go run exif_go.go --input <файл> [--dir] [--output <map.html>]")
		return
	}
	if *dirMode {
		processDirectory(*inputPtr)
		if len(photos) > 0 {
			printInfo(photos[0])
		}
		generateMap(*outputPtr)
	} else {
		data := processFile(*inputPtr)
		printInfo(data)
		if data.HasGPS {
			generateMap(*outputPtr)
		}
	}
}
