<script lang="ts">
  import { currentProfile } from '$lib/stores/profileStore';
  import { searchQuery } from '$lib/stores/libraryStore';

  export let onShowProfileSelector: () => void;

  $: profile = $currentProfile;

  function handleSearchInput(event: Event) {
    const target = event.target as HTMLInputElement;
    searchQuery.set(target.value);
  }
</script>

<header>
  <div class="header-top">
    <h1>Media Server</h1>
    {#if profile}
      <div class="profile-switcher" on:click={onShowProfileSelector} on:keydown={(e) => e.key === 'Enter' && onShowProfileSelector()} role="button" tabindex="0" title="Switch profile">
        <span class="profile-icon">{profile.icon}</span>
        <span class="profile-name">{profile.name}</span>
      </div>
    {/if}
  </div>
  <input type="text" class="search-bar" placeholder="Search videos..." on:input={handleSearchInput} />
</header>

<style lang="scss">
  header {
    background-color: var(--color-bg-secondary);
    padding: var(--spacing-xl);
    border-bottom: 1px solid var(--color-border-primary);
    position: sticky;
    top: 0;
    z-index: var(--z-sticky);
  }

  .header-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: var(--spacing-lg);
    gap: var(--spacing-lg);
  }

  h1 {
    font-size: var(--font-size-2xl);
    margin: 0;
    color: var(--color-text-primary);
    font-weight: var(--font-weight-semibold);
  }

  .profile-switcher {
    display: flex;
    align-items: center;
    gap: var(--spacing-sm);
    padding: var(--spacing-sm) var(--spacing-md);
    background-color: var(--color-bg-hover);
    border: 2px solid var(--color-border-secondary);
    border-radius: var(--radius-md);
    cursor: pointer;
    transition: all var(--transition-fast);
    user-select: none;
    outline: none;

    &:hover {
      background-color: #252525;
      border-color: var(--color-border-tertiary);
    }

    &:focus-visible {
      outline: 2px solid var(--color-border-tertiary);
      outline-offset: 2px;
    }
  }

  .profile-icon {
    font-size: 1.5rem;
    line-height: 1;
  }

  .profile-name {
    font-size: var(--font-size-sm);
    color: var(--color-text-muted);
    font-weight: var(--font-weight-medium);
  }

  .profile-switcher:hover .profile-name {
    color: var(--color-text-primary);
  }

  .search-bar {
    width: 100%;
    max-width: 600px;
    padding: var(--spacing-md) var(--spacing-lg);
    font-size: var(--font-size-base);
    background-color: var(--color-bg-hover);
    border: 1px solid var(--color-border-secondary);
    border-radius: var(--radius-md);
    color: var(--color-text-primary);
    outline: none;
    transition: border-color var(--transition-fast);

    &::placeholder {
      color: var(--color-text-disabled);
    }

    &:focus {
      border-color: var(--color-border-tertiary);
    }
  }

  // Mobile responsive
  @media (max-width: 768px) {
    header {
      padding: var(--spacing-lg);
    }

    .header-top {
      flex-direction: row;
      justify-content: space-between;
    }

    h1 {
      font-size: var(--font-size-xl);
    }

    .profile-switcher {
      padding: 0.4rem 0.6rem;
    }

    .profile-icon {
      font-size: 1.3rem;
    }

    .profile-name {
      font-size: 0.8rem;
    }

    .search-bar {
      font-size: var(--font-size-sm);
    }
  }

  @media (max-width: 480px) {
    h1 {
      font-size: var(--font-size-lg);
    }

    .profile-name {
      display: none; // Hide name on very small screens
    }
  }
</style>
