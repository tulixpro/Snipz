# Snipz

Snipz is a responsive short-video social app starter built with HTML/CSS/JavaScript and Supabase.

## Included

- Email/password registration and sign-in
- Public short-video feed backed by Supabase
- Video upload to Supabase Storage (100 MB limit)
- Likes, comments, follows and profile editing
- Video reporting flow
- Row Level Security policies for the database and storage

## Connect your Supabase project

1. Create a project at https://supabase.com.
2. Open **Project Settings -> API** and copy the **Project URL** and **anon/public key**.
3. Edit `config.js` and replace the two placeholder values. Use only the anon/public key in browser code. **Never use a service_role key in this repository.**
4. In Supabase, open **SQL Editor**, paste and run all of `supabase/schema.sql`.
5. In **Authentication -> URL Configuration**, add the URL where you host Snipz to the allowed redirect/site URLs.
6. Serve the repository using a static host (GitHub Pages, Netlify, Cloudflare Pages, etc.) or a local static server. Because this app uses JavaScript modules, do not open index.html as a `file://` URL.

## Run locally

For example, from the repository root:

```bash
python -m http.server 8000
```

Then open http://localhost:8000.

## Important before a public launch

This is a starter, not a fully production-hardened social network. Before inviting the public:
- Configure email verification, password recovery, rate limits and abuse protection in Supabase.
- Add a real moderation workflow for reviewing reports, removing content and suspending accounts.
- Add age-appropriate privacy defaults, community guidelines, blocking, account deletion and data export.
- Consider private video storage with signed URLs, video transcoding, thumbnail generation and upload scanning.
- Review applicable privacy, child-safety, copyright and platform requirements for your launch region.
- Add automated tests, monitoring, backups and a privacy policy / terms of service.

The current video bucket is public so published clips can play without signing in. Do not upload private or sensitive videos.
