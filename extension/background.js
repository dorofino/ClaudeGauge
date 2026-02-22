// Service worker: provides cookie access to content scripts
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === "getSessionKey") {
    chrome.cookies.get(
      { url: "https://claude.ai", name: "sessionKey" },
      (cookie) => {
        sendResponse({ key: cookie ? cookie.value : null });
      }
    );
    return true; // keep channel open for async response
  }
});
