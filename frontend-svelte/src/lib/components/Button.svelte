<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();

  export let variant: 'primary' | 'secondary' | 'ghost' = 'ghost';
  export let title: string = '';
  export let disabled: boolean = false;

  function handleClick(event: MouseEvent) {
    if (!disabled) {
      dispatch('click', event);
    }
  }
</script>

<button
  class="button"
  class:primary={variant === 'primary'}
  class:secondary={variant === 'secondary'}
  class:ghost={variant === 'ghost'}
  {title}
  {disabled}
  on:click={handleClick}
>
  <slot />
</button>

<style lang="scss">
  .button {
    background: none;
    border: none;
    cursor: pointer;
    padding: var(--spacing-xs) var(--spacing-sm);
    color: var(--color-text-disabled);
    transition: all var(--transition-fast);
    user-select: none;
    min-width: 2rem;
    text-align: center;
    border-radius: var(--radius-sm);
    font-size: 1.2rem;

    &:hover:not(:disabled) {
      background-color: var(--color-bg-hover);
      color: var(--color-text-primary);
    }

    &:active:not(:disabled) {
      transform: scale(0.95);
    }

    &:disabled {
      cursor: not-allowed;
      opacity: 0.5;
    }

    &.primary {
      color: var(--color-accent-success);

      &:hover:not(:disabled) {
        color: var(--color-accent-success-bright);
        background-color: var(--color-accent-success-bg);
      }
    }

    &.secondary {
      background-color: var(--color-bg-secondary);
      border: 1px solid var(--color-border-secondary);
      color: var(--color-text-primary);
      padding: var(--spacing-sm) var(--spacing-lg);
      font-size: var(--font-size-base);

      &:hover:not(:disabled) {
        background-color: var(--color-bg-tertiary);
        border-color: var(--color-border-tertiary);
      }
    }
  }
</style>
