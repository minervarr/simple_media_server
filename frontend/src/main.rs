use yew::prelude::*;
use serde::{Deserialize, Serialize};
use gloo_net::http::Request;
use web_sys::HtmlInputElement;
use gloo_storage::{LocalStorage, Storage};
use std::collections::HashSet;

// Data structures matching backend JSON
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
    search_query: String,
    expanded_series: Vec<String>,
    expanded_seasons: Vec<String>,
    loading: bool,
    error: Option<String>,
    playing_video: Option<VideoInfo>,
    watched_videos: HashSet<String>,
}

#[derive(Clone, PartialEq)]
struct VideoInfo {
    url: String,
    title: String,
}

enum Msg {
    LibraryLoaded(Library),
    LoadError(String),
    UpdateSearch(String),
    ToggleSeries(String),
    ToggleSeason(String, u32),
    PlayVideo(String, String), // (url, title)
    CloseVideo,
    ToggleWatched(String), // video path
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(ctx: &Context<Self>) -> Self {
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

        // Load watched videos from localStorage
        let watched_videos: HashSet<String> = LocalStorage::get("watched_videos").unwrap_or_default();

        Self {
            library: None,
            search_query: String::new(),
            expanded_series: Vec::new(),
            expanded_seasons: Vec::new(),
            loading: true,
            error: None,
            playing_video: None,
            watched_videos,
        }
    }

    fn update(&mut self, _ctx: &Context<Self>, msg: Self::Message) -> bool {
        match msg {
            Msg::LibraryLoaded(library) => {
                self.library = Some(library);
                self.loading = false;
                true
            }
            Msg::LoadError(error) => {
                self.error = Some(error);
                self.loading = false;
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
            Msg::PlayVideo(url, title) => {
                // Extract the video path from the URL (remove "/video/" prefix)
                let path = url.strip_prefix("/video/").unwrap_or(&url).to_string();

                // Mark as watched when playing
                self.watched_videos.insert(path);
                let _ = LocalStorage::set("watched_videos", &self.watched_videos);

                self.playing_video = Some(VideoInfo { url, title });
                true
            }
            Msg::CloseVideo => {
                self.playing_video = None;
                true
            }
            Msg::ToggleWatched(path) => {
                if self.watched_videos.contains(&path) {
                    self.watched_videos.remove(&path);
                } else {
                    self.watched_videos.insert(path);
                }
                let _ = LocalStorage::set("watched_videos", &self.watched_videos);
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

                <header>
                    <h1>{"Media Server"}</h1>
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
        let url_clone = video_url.clone();
        let path = episode.path.clone();
        let path_for_toggle = path.clone();
        let is_watched = self.watched_videos.contains(&path);

        html! {
            <div class={classes!("episode", is_watched.then_some("watched"))}>
                <div
                    class="episode-content"
                    onclick={ctx.link().callback(move |_| {
                        Msg::PlayVideo(url_clone.clone(), title.clone())
                    })}
                >
                    {if let Some(ep_num) = episode.episode {
                        html! { <span class="episode-number">{format!("E{:02}", ep_num)}</span> }
                    } else {
                        html! {}
                    }}
                    <span class="episode-name">{&episode.filename}</span>
                </div>
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
        }
    }

    fn render_movie(&self, ctx: &Context<Self>, movie: &Movie) -> Html {
        let video_url = format!("/video/{}", movie.path);
        let title = movie.name.clone();
        let url_clone = video_url.clone();
        let path = movie.path.clone();
        let path_for_toggle = path.clone();
        let is_watched = self.watched_videos.contains(&path);

        html! {
            <div class={classes!("movie", is_watched.then_some("watched"))}>
                <div
                    class="movie-content"
                    onclick={ctx.link().callback(move |_| {
                        Msg::PlayVideo(url_clone.clone(), title.clone())
                    })}
                >
                    <div class="movie-name">{&movie.name}</div>
                </div>
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
        }
    }

    fn render_video_player(&self, ctx: &Context<Self>) -> Html {
        if let Some(video) = &self.playing_video {
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
                            controls=true
                            autoplay=true
                            preload="metadata"
                            src={video.url.clone()}
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
