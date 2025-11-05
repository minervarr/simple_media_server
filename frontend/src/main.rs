use yew::prelude::*;
use serde::{Deserialize, Serialize};
use gloo_net::http::Request;
use web_sys::HtmlInputElement;
use gloo_storage::{LocalStorage, Storage};
use std::collections::HashSet;
use wasm_bindgen::JsCast;
use web_sys::{window, Navigator};

// Data structures matching backend JSON
#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Profile {
    id: String,
    name: String,
    icon: String,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Video {
    path: String,
    filename: String,
    episode: Option<u32>,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Season {
    number: u32,
    episodes: Vec<Video>,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Series {
    name: String,
    #[serde(rename = "displayName")]
    display_name: String,
    seasons: Vec<Season>,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Movie {
    name: String,
    path: String,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
struct Library {
    series: Vec<Series>,
    movies: Vec<Movie>,
}

// Component state
struct App {
    library: Option<Library>,
    profiles: Vec<Profile>,
    current_profile: Option<Profile>,
    show_profile_selector: bool,
    search_query: String,
    expanded_series: Vec<String>,
    expanded_seasons: Vec<String>,
    loading: bool,
    profiles_loading: bool,
    error: Option<String>,
    watched_videos: HashSet<String>,
    copied_video: Option<String>, // Track which video was just copied
    playing_video: Option<VideoInfo>,
}

#[derive(Clone, PartialEq)]
struct VideoInfo {
    path: String,
    title: String,
}

enum Msg {
    LibraryLoaded(Library),
    ProfilesLoaded(Vec<Profile>),
    LoadError(String),
    UpdateSearch(String),
    ToggleSeries(String),
    ToggleSeason(String, u32),
    PlayVideo(String, String), // (path, title)
    CloseVideo,
    CopyVideoLink(String, String), // (url, path)
    ShareVideo(String, String), // (url, title)
    ToggleWatched(String), // video path
    SelectProfile(Profile),
    ShowProfileSelector,
    ClearCopiedState,
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(ctx: &Context<Self>) -> Self {
        // Fetch profiles on startup
        ctx.link().send_future(async {
            match Request::get("/api/profiles").send().await {
                Ok(response) => {
                    match response.json::<Vec<Profile>>().await {
                        Ok(profiles) => Msg::ProfilesLoaded(profiles),
                        Err(e) => Msg::LoadError(format!("Failed to parse profiles: {}", e)),
                    }
                }
                Err(e) => Msg::LoadError(format!("Failed to fetch profiles: {}", e)),
            }
        });

        // Fetch library data on startup
        ctx.link().send_future(async {
            match Request::get("/api/library").send().await {
                Ok(response) => {
                    match response.json::<Library>().await {
                        Ok(library) => Msg::LibraryLoaded(library),
                        Err(e) => Msg::LoadError(format!("Failed to parse library: {}", e)),
                    }
                }
                Err(e) => Msg::LoadError(format!("Failed to fetch library: {}", e)),
            }
        });

        Self {
            library: None,
            profiles: Vec::new(),
            current_profile: None,
            show_profile_selector: true, // Will be set to false if we have a saved profile
            search_query: String::new(),
            expanded_series: Vec::new(),
            expanded_seasons: Vec::new(),
            loading: true,
            profiles_loading: true,
            error: None,
            watched_videos: HashSet::new(),
            copied_video: None,
            playing_video: None,
        }
    }

    fn update(&mut self, _ctx: &Context<Self>, msg: Self::Message) -> bool {
        match msg {
            Msg::LibraryLoaded(library) => {
                self.library = Some(library);
                self.loading = false;
                true
            }
            Msg::ProfilesLoaded(profiles) => {
                self.profiles = profiles;
                self.profiles_loading = false;

                // Try to load last selected profile from localStorage
                if let Ok(last_profile_id) = LocalStorage::get::<String>("current_profile_id") {
                    if let Some(profile) = self.profiles.iter().find(|p| p.id == last_profile_id) {
                        self.current_profile = Some(profile.clone());
                        self.show_profile_selector = false; // Hide selector if we have a saved profile

                        // Load watched videos for current profile
                        let storage_key = format!("watched_videos_{}", profile.id);
                        self.watched_videos = LocalStorage::get(&storage_key).unwrap_or_default();
                    }
                }

                // If no profile selected, show profile selector
                if self.current_profile.is_none() {
                    self.show_profile_selector = true;
                }

                true
            }
            Msg::LoadError(error) => {
                self.error = Some(error);
                self.loading = false;
                self.profiles_loading = false;
                true
            }
            Msg::UpdateSearch(query) => {
                self.search_query = query.to_lowercase();
                true
            }
            Msg::ToggleSeries(series_name) => {
                if let Some(pos) = self.expanded_series.iter().position(|s| s == &series_name) {
                    self.expanded_series.remove(pos);
                } else {
                    self.expanded_series.push(series_name);
                }
                true
            }
            Msg::ToggleSeason(series_name, season_num) => {
                let key = format!("{}_{}", series_name, season_num);
                if let Some(pos) = self.expanded_seasons.iter().position(|s| s == &key) {
                    self.expanded_seasons.remove(pos);
                } else {
                    self.expanded_seasons.push(key);
                }
                true
            }
            Msg::PlayVideo(path, title) => {
                // Mark as watched when playing
                self.watched_videos.insert(path.clone());
                if let Some(profile) = &self.current_profile {
                    let storage_key = format!("watched_videos_{}", profile.id);
                    let _ = LocalStorage::set(&storage_key, &self.watched_videos);
                }

                self.playing_video = Some(VideoInfo { path, title });
                true
            }
            Msg::CloseVideo => {
                self.playing_video = None;
                true
            }
            Msg::CopyVideoLink(url, path) => {
                // Try to copy to clipboard
                if let Some(window) = window() {
                    if let Some(navigator) = window.navigator() {
                        if let Ok(clipboard) = navigator.clipboard() {
                            let full_url = format!("{}{}", window.location().origin().unwrap_or_default(), url);
                            let _ = clipboard.write_text(&full_url);
                            self.copied_video = Some(path.clone());

                            // Clear the copied state after 2 seconds
                            let link = _ctx.link().clone();
                            wasm_bindgen_futures::spawn_local(async move {
                                gloo_timers::future::TimeoutFuture::new(2000).await;
                                link.send_message(Msg::ClearCopiedState);
                            });
                        }
                    }
                }
                true
            }
            Msg::ShareVideo(url, title) => {
                // Try to use Web Share API (mobile devices)
                if let Some(window) = window() {
                    if let Some(navigator) = window.navigator() {
                        let full_url = format!("{}{}", window.location().origin().unwrap_or_default(), url);

                        // Use share API if available
                        let share_data = js_sys::Object::new();
                        js_sys::Reflect::set(&share_data, &"title".into(), &title.into()).ok();
                        js_sys::Reflect::set(&share_data, &"url".into(), &full_url.into()).ok();

                        if let Ok(share) = js_sys::Reflect::get(&navigator, &"share".into()) {
                            if !share.is_undefined() {
                                let _ = js_sys::Reflect::apply(
                                    &share.unchecked_into(),
                                    &navigator.into(),
                                    &js_sys::Array::of1(&share_data)
                                );
                            }
                        }
                    }
                }
                true
            }
            Msg::ClearCopiedState => {
                self.copied_video = None;
                true
            }
            Msg::ToggleWatched(path) => {
                if self.watched_videos.contains(&path) {
                    self.watched_videos.remove(&path);
                } else {
                    self.watched_videos.insert(path);
                }
                // Save to profile-specific storage
                if let Some(profile) = &self.current_profile {
                    let storage_key = format!("watched_videos_{}", profile.id);
                    let _ = LocalStorage::set(&storage_key, &self.watched_videos);
                }
                true
            }
            Msg::SelectProfile(profile) => {
                // Save watched videos for current profile
                if let Some(current) = &self.current_profile {
                    let storage_key = format!("watched_videos_{}", current.id);
                    let _ = LocalStorage::set(&storage_key, &self.watched_videos);
                }

                // Switch to new profile
                self.current_profile = Some(profile.clone());
                let _ = LocalStorage::set("current_profile_id", &profile.id);

                // Load watched videos for new profile
                let storage_key = format!("watched_videos_{}", profile.id);
                self.watched_videos = LocalStorage::get(&storage_key).unwrap_or_default();

                // Hide profile selector after selection
                self.show_profile_selector = false;

                true
            }
            Msg::ShowProfileSelector => {
                self.show_profile_selector = true;
                true
            }
        }
    }

    fn view(&self, ctx: &Context<Self>) -> Html {
        html! {
            <div class="app">
                <style>
                    {include_str!("../style.css")}
                </style>

                {if self.show_profile_selector {
                    self.render_profile_selection_screen(ctx)
                } else {
                    html! {
                        <>
                            <header>
                                <div class="header-top">
                                    <h1>{"Media Server"}</h1>
                                    {self.render_profile_switcher_button(ctx)}
                                </div>
                                <input
                                    type="text"
                                    class="search-bar"
                                    placeholder="Search videos..."
                                    oninput={ctx.link().callback(|e: InputEvent| {
                                        let input: HtmlInputElement = e.target_unchecked_into();
                                        Msg::UpdateSearch(input.value())
                                    })}
                                />
                            </header>

                            <main>
                                {self.render_content(ctx)}
                            </main>

                            {self.render_video_player(ctx)}
                        </>
                    }
                }}
            </div>
        }
    }
}

impl App {
    fn render_content(&self, ctx: &Context<Self>) -> Html {
        if self.loading {
            return html! { <div class="loading">{"Loading library..."}</div> };
        }

        if let Some(error) = &self.error {
            return html! { <div class="error">{format!("Error: {}", error)}</div> };
        }

        if let Some(library) = &self.library {
            let filtered_series = self.filter_series(&library.series);
            let filtered_movies = self.filter_movies(&library.movies);

            html! {
                <>
                    {if !filtered_series.is_empty() {
                        html! {
                            <section class="series-section">
                                <h2>{"TV Series"}</h2>
                                {for filtered_series.iter().map(|s| self.render_series(ctx, s))}
                            </section>
                        }
                    } else {
                        html! {}
                    }}

                    {if !filtered_movies.is_empty() {
                        html! {
                            <section class="movies-section">
                                <h2>{"Movies"}</h2>
                                <div class="movie-list">
                                    {for filtered_movies.iter().map(|m| self.render_movie(ctx, m))}
                                </div>
                            </section>
                        }
                    } else {
                        html! {}
                    }}

                    {if filtered_series.is_empty() && filtered_movies.is_empty() {
                        html! { <div class="no-results">{"No results found"}</div> }
                    } else {
                        html! {}
                    }}
                </>
            }
        } else {
            html! { <div class="error">{"No library data available"}</div> }
        }
    }

    fn filter_series(&self, series: &[Series]) -> Vec<Series> {
        if self.search_query.is_empty() {
            return series.to_vec();
        }

        series
            .iter()
            .filter(|s| {
                s.display_name.to_lowercase().contains(&self.search_query)
                    || s.name.to_lowercase().contains(&self.search_query)
            })
            .cloned()
            .collect()
    }

    fn filter_movies(&self, movies: &[Movie]) -> Vec<Movie> {
        if self.search_query.is_empty() {
            return movies.to_vec();
        }

        movies
            .iter()
            .filter(|m| m.name.to_lowercase().contains(&self.search_query))
            .cloned()
            .collect()
    }

    fn render_series(&self, ctx: &Context<Self>, series: &Series) -> Html {
        let is_expanded = self.expanded_series.contains(&series.name);
        let series_name = series.name.clone();

        html! {
            <div class="series">
                <div
                    class="series-header"
                    onclick={ctx.link().callback(move |_| Msg::ToggleSeries(series_name.clone()))}
                >
                    <span class="expand-icon">{if is_expanded { "▼" } else { "▶" }}</span>
                    <span class="series-name">{&series.display_name}</span>
                    <span class="series-count">{format!("{} seasons", series.seasons.len())}</span>
                </div>

                {if is_expanded {
                    html! {
                        <div class="seasons">
                            {for series.seasons.iter().map(|season| self.render_season(ctx, &series.name, season))}
                        </div>
                    }
                } else {
                    html! {}
                }}
            </div>
        }
    }

    fn render_season(&self, ctx: &Context<Self>, series_name: &str, season: &Season) -> Html {
        let season_key = format!("{}_{}", series_name, season.number);
        let is_expanded = self.expanded_seasons.contains(&season_key);
        let series_name_clone = series_name.to_string();
        let season_num = season.number;

        html! {
            <div class="season">
                <div
                    class="season-header"
                    onclick={ctx.link().callback(move |_| {
                        Msg::ToggleSeason(series_name_clone.clone(), season_num)
                    })}
                >
                    <span class="expand-icon">{if is_expanded { "▼" } else { "▶" }}</span>
                    <span class="season-name">{format!("Season {}", season.number)}</span>
                    <span class="episode-count">{format!("{} episodes", season.episodes.len())}</span>
                </div>

                {if is_expanded {
                    html! {
                        <div class="episodes">
                            {for season.episodes.iter().map(|ep| self.render_episode(ctx, ep))}
                        </div>
                    }
                } else {
                    html! {}
                }}
            </div>
        }
    }

    fn render_episode(&self, ctx: &Context<Self>, episode: &Video) -> Html {
        let video_url = format!("/video/{}", episode.path);
        let title = episode.filename.clone();
        let path = episode.path.clone();
        let is_watched = self.watched_videos.contains(&path);
        let is_copied = self.copied_video.as_ref() == Some(&path);

        let path_for_play = path.clone();
        let title_for_play = title.clone();
        let url_for_copy = video_url.clone();
        let path_for_copy = path.clone();
        let url_for_share = video_url.clone();
        let title_for_share = title.clone();
        let path_for_toggle = path.clone();

        html! {
            <div class={classes!("episode", is_watched.then_some("watched"))}>
                <div class="episode-info">
                    {if let Some(ep_num) = episode.episode {
                        html! { <span class="episode-number">{format!("E{:02}", ep_num)}</span> }
                    } else {
                        html! {}
                    }}
                    <span class="episode-name">{&episode.filename}</span>
                </div>
                <div class="episode-actions">
                    <button
                        class="action-button play-button"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::PlayVideo(path_for_play.clone(), title_for_play.clone())
                        })}
                        title="Play"
                    >
                        {"▶"}
                    </button>
                    <button
                        class={classes!("action-button", "copy-button", is_copied.then_some("copied"))}
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::CopyVideoLink(url_for_copy.clone(), path_for_copy.clone())
                        })}
                        title={if is_copied { "Link copied!" } else { "Copy link" }}
                    >
                        {if is_copied { "✓" } else { "🔗" }}
                    </button>
                    <button
                        class="action-button share-button"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::ShareVideo(url_for_share.clone(), title_for_share.clone())
                        })}
                        title="Share"
                    >
                        {"📤"}
                    </button>
                    <div
                        class="watched-indicator"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::ToggleWatched(path_for_toggle.clone())
                        })}
                        title={if is_watched { "Mark as unwatched" } else { "Mark as watched" }}
                    >
                        {if is_watched { "✓" } else { "○" }}
                    </div>
                </div>
            </div>
        }
    }

    fn render_movie(&self, ctx: &Context<Self>, movie: &Movie) -> Html {
        let video_url = format!("/video/{}", movie.path);
        let title = movie.name.clone();
        let path = movie.path.clone();
        let is_watched = self.watched_videos.contains(&path);
        let is_copied = self.copied_video.as_ref() == Some(&path);

        let path_for_play = path.clone();
        let title_for_play = title.clone();
        let url_for_copy = video_url.clone();
        let path_for_copy = path.clone();
        let url_for_share = video_url.clone();
        let title_for_share = title.clone();
        let path_for_toggle = path.clone();

        html! {
            <div class={classes!("movie", is_watched.then_some("watched"))}>
                <div class="movie-name">{&movie.name}</div>
                <div class="movie-actions">
                    <button
                        class="action-button play-button"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::PlayVideo(path_for_play.clone(), title_for_play.clone())
                        })}
                        title="Play"
                    >
                        {"▶"}
                    </button>
                    <button
                        class={classes!("action-button", "copy-button", is_copied.then_some("copied"))}
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::CopyVideoLink(url_for_copy.clone(), path_for_copy.clone())
                        })}
                        title={if is_copied { "Link copied!" } else { "Copy link" }}
                    >
                        {if is_copied { "✓" } else { "🔗" }}
                    </button>
                    <button
                        class="action-button share-button"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::ShareVideo(url_for_share.clone(), title_for_share.clone())
                        })}
                        title="Share"
                    >
                        {"📤"}
                    </button>
                    <div
                        class="watched-indicator movie-watched"
                        onclick={ctx.link().callback(move |e: MouseEvent| {
                            e.stop_propagation();
                            Msg::ToggleWatched(path_for_toggle.clone())
                        })}
                        title={if is_watched { "Mark as unwatched" } else { "Mark as watched" }}
                    >
                        {if is_watched { "✓" } else { "○" }}
                    </div>
                </div>
            </div>
        }
    }

    fn render_profile_selection_screen(&self, ctx: &Context<Self>) -> Html {
        if self.profiles_loading {
            return html! {
                <div class="profile-selection-screen">
                    <div class="loading">{"Loading profiles..."}</div>
                </div>
            };
        }

        if self.profiles.is_empty() {
            return html! {
                <div class="profile-selection-screen">
                    <div class="error">{"No profiles configured"}</div>
                </div>
            };
        }

        html! {
            <div class="profile-selection-screen">
                <div class="profile-selection-content">
                    <h1 class="profile-selection-title">{"Who's watching?"}</h1>
                    <div class="profile-grid">
                        {for self.profiles.iter().map(|profile| {
                            let profile_clone = profile.clone();

                            html! {
                                <div
                                    class="profile-card"
                                    onclick={ctx.link().callback(move |_| {
                                        Msg::SelectProfile(profile_clone.clone())
                                    })}
                                >
                                    <div class="profile-avatar">
                                        <span class="profile-avatar-icon">{&profile.icon}</span>
                                    </div>
                                    <div class="profile-card-name">{&profile.name}</div>
                                </div>
                            }
                        })}
                    </div>
                </div>
            </div>
        }
    }

    fn render_profile_switcher_button(&self, ctx: &Context<Self>) -> Html {
        if let Some(profile) = &self.current_profile {
            html! {
                <div
                    class="profile-switcher"
                    onclick={ctx.link().callback(|_| Msg::ShowProfileSelector)}
                    title="Switch profile"
                >
                    <span class="profile-switcher-icon">{&profile.icon}</span>
                    <span class="profile-switcher-name">{&profile.name}</span>
                </div>
            }
        } else {
            html! {}
        }
    }

    fn render_video_player(&self, ctx: &Context<Self>) -> Html {
        if let Some(video) = &self.playing_video {
            // Build HLS playlist URL
            let hls_url = format!("/hls/{}/playlist.m3u8", video.path);

            html! {
                <div class="video-player-overlay">
                    <div class="video-player-container">
                        <div class="video-player-header">
                            <h3>{&video.title}</h3>
                            <button
                                class="close-button"
                                onclick={ctx.link().callback(|_| Msg::CloseVideo)}
                            >
                                {"✕"}
                            </button>
                        </div>
                        <video
                            id="hls-video"
                            class="hls-video"
                            controls=true
                            autoplay=true
                            data-hls-url={hls_url}
                        />
                    </div>
                </div>
            }
        } else {
            html! {}
        }
    }
}

fn main() {
    yew::Renderer::<App>::new().render();
}
