-- Snipz database setup for Supabase
-- Run the complete file in Supabase Dashboard -> SQL Editor.
create extension if not exists pgcrypto;

create table if not exists public.profiles (
  id uuid primary key references auth.users(id) on delete cascade,
  username text not null unique check (username ~ '^[a-z0-9_]{3,24}$'),
  display_name text not null default '',
  avatar_url text,
  bio text not null default '' check (char_length(bio) <= 160),
  created_at timestamptz not null default now()
);

create table if not exists public.videos (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references public.profiles(id) on delete cascade,
  storage_path text not null unique,
  caption text not null default '' check (char_length(caption) <= 500),
  visibility text not null default 'public' check (visibility in ('public','private')),
  created_at timestamptz not null default now()
);
create index if not exists videos_created_at_idx on public.videos(created_at desc);
create index if not exists videos_user_id_idx on public.videos(user_id);

create table if not exists public.likes (
  video_id uuid not null references public.videos(id) on delete cascade,
  user_id uuid not null references public.profiles(id) on delete cascade,
  created_at timestamptz not null default now(),
  primary key(video_id,user_id)
);
create table if not exists public.comments (
  id uuid primary key default gen_random_uuid(),
  video_id uuid not null references public.videos(id) on delete cascade,
  user_id uuid not null references public.profiles(id) on delete cascade,
  body text not null check (char_length(body) between 1 and 500),
  created_at timestamptz not null default now()
);
create index if not exists comments_video_created_idx on public.comments(video_id,created_at);
create table if not exists public.follows (
  follower_id uuid not null references public.profiles(id) on delete cascade,
  following_id uuid not null references public.profiles(id) on delete cascade,
  created_at timestamptz not null default now(),
  primary key(follower_id,following_id),
  check(follower_id <> following_id)
);
create table if not exists public.reports (
  id uuid primary key default gen_random_uuid(),
  reporter_id uuid not null references public.profiles(id) on delete cascade,
  video_id uuid references public.videos(id) on delete set null,
  reason text not null check (char_length(reason) between 1 and 500),
  status text not null default 'open' check (status in ('open','reviewing','resolved','dismissed')),
  created_at timestamptz not null default now()
);

-- Automatically create a profile when an account is registered.
create or replace function public.handle_new_user()
returns trigger language plpgsql security definer set search_path = '' as $$
declare
  candidate text;
begin
  candidate := lower(regexp_replace(coalesce(new.raw_user_meta_data->>'username', split_part(new.email,'@',1)), '[^a-z0-9_]', '', 'g'));
  candidate := left(candidate,20);
  if length(candidate) < 3 then candidate := 'user_' || left(new.id::text,8); end if;
  insert into public.profiles(id,username,display_name)
  values(new.id,candidate,coalesce(new.raw_user_meta_data->>'username',candidate))
  on conflict (id) do nothing;
  return new;
end;
$$;
drop trigger if exists on_auth_user_created on auth.users;
create trigger on_auth_user_created after insert on auth.users
for each row execute procedure public.handle_new_user();

alter table public.profiles enable row level security;
alter table public.videos enable row level security;
alter table public.likes enable row level security;
alter table public.comments enable row level security;
alter table public.follows enable row level security;
alter table public.reports enable row level security;

drop policy if exists "Profiles are publicly readable" on public.profiles;
create policy "Profiles are publicly readable" on public.profiles for select using (true);
drop policy if exists "Users can update own profile" on public.profiles;
create policy "Users can update own profile" on public.profiles for update to authenticated using (auth.uid() = id) with check (auth.uid() = id);

drop policy if exists "Public videos are readable" on public.videos;
create policy "Public videos are readable" on public.videos for select using (visibility = 'public' or auth.uid() = user_id);
drop policy if exists "Users can publish own videos" on public.videos;
create policy "Users can publish own videos" on public.videos for insert to authenticated with check (auth.uid() = user_id);
drop policy if exists "Users can delete own videos" on public.videos;
create policy "Users can delete own videos" on public.videos for delete to authenticated using (auth.uid() = user_id);

drop policy if exists "Likes are publicly readable" on public.likes;
create policy "Likes are publicly readable" on public.likes for select using (true);
drop policy if exists "Users can like as themselves" on public.likes;
create policy "Users can like as themselves" on public.likes for insert to authenticated with check (auth.uid() = user_id);
drop policy if exists "Users can remove own likes" on public.likes;
create policy "Users can remove own likes" on public.likes for delete to authenticated using (auth.uid() = user_id);

drop policy if exists "Comments are publicly readable" on public.comments;
create policy "Comments are publicly readable" on public.comments for select using (true);
drop policy if exists "Signed-in users can comment as themselves" on public.comments;
create policy "Signed-in users can comment as themselves" on public.comments for insert to authenticated with check (auth.uid() = user_id);
drop policy if exists "Users can delete own comments" on public.comments;
create policy "Users can delete own comments" on public.comments for delete to authenticated using (auth.uid() = user_id);

drop policy if exists "Follows are publicly readable" on public.follows;
create policy "Follows are publicly readable" on public.follows for select using (true);
drop policy if exists "Users can follow as themselves" on public.follows;
create policy "Users can follow as themselves" on public.follows for insert to authenticated with check (auth.uid() = follower_id and follower_id <> following_id);
drop policy if exists "Users can unfollow themselves" on public.follows;
create policy "Users can unfollow themselves" on public.follows for delete to authenticated using (auth.uid() = follower_id);

-- Reports are private: reporters can create and read only their own reports.
drop policy if exists "Users can submit reports" on public.reports;
create policy "Users can submit reports" on public.reports for insert to authenticated with check (auth.uid() = reporter_id);
drop policy if exists "Users can read own reports" on public.reports;
create policy "Users can read own reports" on public.reports for select to authenticated using (auth.uid() = reporter_id);

-- Public video bucket; database RLS still controls which records appear in the feed.
insert into storage.buckets(id,name,public,file_size_limit,allowed_mime_types)
values('videos','videos',true,104857600,array['video/mp4','video/webm','video/quicktime'])
on conflict (id) do update set public = true, file_size_limit = 104857600,
allowed_mime_types = array['video/mp4','video/webm','video/quicktime'];

drop policy if exists "Anyone can view published video files" on storage.objects;
create policy "Anyone can view published video files" on storage.objects for select using (bucket_id = 'videos');
drop policy if exists "Users upload into their own folder" on storage.objects;
create policy "Users upload into their own folder" on storage.objects for insert to authenticated
with check (bucket_id = 'videos' and (storage.foldername(name))[1] = auth.uid()::text);
drop policy if exists "Users delete files in their own folder" on storage.objects;
create policy "Users delete files in their own folder" on storage.objects for delete to authenticated
using (bucket_id = 'videos' and (storage.foldername(name))[1] = auth.uid()::text);

-- Expose the public schema to the Supabase Data API if it is not already exposed:
-- Dashboard -> Project Settings -> API -> Exposed schemas -> public.
