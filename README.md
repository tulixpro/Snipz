# Snipz

Snipz ist eine Kurzvideo-Plattform mit vertikalem Feed, Profilen, Likes, Kommentaren und Uploads.

## Projektstatus

Die Weboberfläche und das Cloudflare-Server-Grundgerüst liegen in diesem Repository. Der Server ist noch nicht live geschaltet. Die bisherigen Konten- und Medienfunktionen werden erst nach vollständiger Umstellung und Sicherheitstests aktiviert.

## Geplante Infrastruktur

- Cloudflare Pages für die Website
- Cloudflare Workers für die API
- Cloudflare D1 als verwaltete Datenbank
- Cloudflare R2 für Video- und Bilddateien
- Turnstile, Rate Limits und Upload-Quoten gegen automatisierten Missbrauch

## Sicherheit

- Keine Datenbankpasswörter oder privaten Schlüssel im Browser oder Repository.
- Anmeldung und Rollen werden serverseitig geprüft.
- Admin-Rechte werden niemals aus Browser-Eingaben übernommen.
- Uploads erhalten Größen- und Dateitypgrenzen sowie kurzlebige, serverseitig autorisierte Upload-Freigaben.
- Moderation, Meldungen, Audit-Protokolle und Kontosperren müssen serverseitig umgesetzt und getestet werden.
- Followerzahlen und automatische Verifizierung ab 1.000.000 Followern werden ausschließlich aus vertrauenswürdigen Daten berechnet.

## Lokaler Start

Für die reine Weboberfläche kann ein statischer Server verwendet werden:

```bash
python -m http.server 8000
```

Dann http://localhost:8000 öffnen.

## Cloudflare-Einrichtung

Siehe [cloudflare/README.md](cloudflare/README.md). Die dort beschriebene Worker-Datei ist aktuell nur ein Health-Check-Grundgerüst. Sie bietet noch keine fertige Anmeldung, Uploads, Feed-API oder Admin-Funktionen.

Vor dem öffentlichen Start sind unter anderem Authentifizierung, Autorisierung, Upload-Schutz, Moderation, Backups, Monitoring, Datenschutz und Wiederherstellungstests erforderlich.
