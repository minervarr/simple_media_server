/**
 * Copy text to clipboard with Android fallback
 * Uses modern Clipboard API with document.execCommand fallback for Android
 */
export async function copyToClipboard(text: string): Promise<boolean> {
  const fullUrl = `${window.location.origin}${text}`;

  console.log('Copying URL:', fullUrl);

  // Try Clipboard API (async, may not work on Android)
  try {
    await navigator.clipboard.writeText(fullUrl);
    console.log('Clipboard API worked');
    return true;
  } catch (error) {
    console.log('Clipboard API failed:', error);
  }

  // Fallback: Use textarea + execCommand (synchronous, works on Android)
  try {
    const textarea = document.createElement('textarea');
    textarea.value = fullUrl;
    textarea.style.position = 'absolute';
    textarea.style.left = '-9999px';

    document.body.appendChild(textarea);
    textarea.focus();
    textarea.select();
    textarea.setSelectionRange(0, 999999);

    const success = document.execCommand('copy');
    document.body.removeChild(textarea);

    console.log('execCommand result:', success);
    return success;
  } catch (error) {
    console.error('Fallback copy failed:', error);
    return false;
  }
}

/**
 * Share content using Web Share API (mobile devices)
 * Falls back to clipboard if not available
 */
export async function shareContent(url: string, title: string): Promise<boolean> {
  const fullUrl = `${window.location.origin}${url}`;

  // Check if Web Share API is available
  if (navigator.share) {
    try {
      await navigator.share({
        title: title,
        text: `Watch: ${title}`,
        url: fullUrl,
      });
      console.log('Share successful');
      return true;
    } catch (error) {
      // User cancelled or error occurred
      console.log('Share failed or cancelled:', error);
      return false;
    }
  } else {
    // Fallback to clipboard
    console.log('Share API not available, using clipboard');
    return await copyToClipboard(url);
  }
}
