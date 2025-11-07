<script lang="ts">
  import { profiles, profilesLoading } from '$lib/stores/profileStore';
  import type { Profile } from '$lib/types';

  export let onSelect: (profile: Profile) => void;

  $: loading = $profilesLoading;
  $: profileList = $profiles;
</script>

<div class="profile-selection-screen">
  {#if loading}
    <div class="loading">Loading profiles...</div>
  {:else if profileList.length === 0}
    <div class="error">No profiles configured</div>
  {:else}
    <div class="profile-selection-content">
      <h1 class="title">Who's watching?</h1>
      <div class="profile-grid">
        {#each profileList as profile (profile.id)}
          <div class="profile-card" on:click={() => onSelect(profile)} on:keydown={(e) => e.key === 'Enter' && onSelect(profile)} role="button" tabindex="0">
            <div class="profile-avatar">
              <span class="profile-icon">{profile.icon}</span>
            </div>
            <div class="profile-name">{profile.name}</div>
          </div>
        {/each}
      </div>
    </div>
  {/if}
</div>

<style lang="scss">
  .profile-selection-screen {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: var(--color-bg-primary);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: var(--z-overlay);
  }

  .loading,
  .error {
    text-align: center;
    padding: var(--spacing-3xl);
    color: var(--color-text-muted);
    font-size: var(--font-size-md);
  }

  .error {
    color: var(--color-accent-error);
  }

  .profile-selection-content {
    text-align: center;
    max-width: 900px;
    width: 100%;
    padding: var(--spacing-2xl);
  }

  .title {
    font-size: var(--font-size-4xl);
    margin-bottom: var(--spacing-3xl);
    color: var(--color-text-primary);
    font-weight: var(--font-weight-normal);
  }

  .profile-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 180px));
    gap: var(--spacing-2xl);
    justify-content: center;
  }

  .profile-card {
    cursor: pointer;
    transition: transform var(--transition-fast);
    user-select: none;
    outline: none;

    &:hover {
      transform: scale(1.1);
    }

    &:focus-visible {
      outline: 2px solid var(--color-border-tertiary);
      outline-offset: 4px;
      border-radius: var(--radius-md);
    }
  }

  .profile-avatar {
    width: 150px;
    height: 150px;
    background-color: var(--color-bg-hover);
    border: 3px solid var(--color-border-secondary);
    border-radius: var(--radius-md);
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: var(--spacing-md);
    transition: border-color var(--transition-fast);
  }

  .profile-card:hover .profile-avatar {
    border-color: var(--color-text-primary);
  }

  .profile-icon {
    font-size: 4rem;
    line-height: 1;
  }

  .profile-name {
    font-size: var(--font-size-lg);
    color: var(--color-text-muted);
    font-weight: var(--font-weight-normal);
    transition: color var(--transition-fast);
  }

  .profile-card:hover .profile-name {
    color: var(--color-text-primary);
  }

  // Mobile responsive
  @media (max-width: 768px) {
    .title {
      font-size: var(--font-size-3xl);
      margin-bottom: var(--spacing-2xl);
    }

    .profile-grid {
      grid-template-columns: repeat(auto-fit, minmax(120px, 150px));
      gap: var(--spacing-xl);
    }

    .profile-avatar {
      width: 120px;
      height: 120px;
    }

    .profile-icon {
      font-size: 3rem;
    }

    .profile-name {
      font-size: var(--font-size-base);
    }
  }

  @media (max-width: 480px) {
    .title {
      font-size: var(--font-size-2xl);
      margin-bottom: var(--spacing-xl);
    }

    .profile-grid {
      grid-template-columns: repeat(auto-fit, minmax(100px, 120px));
      gap: var(--spacing-lg);
    }

    .profile-avatar {
      width: 100px;
      height: 100px;
    }

    .profile-icon {
      font-size: 2.5rem;
    }

    .profile-name {
      font-size: var(--font-size-sm);
    }
  }
</style>
