const content = document.getElementById("content");

async function getSessionKey() {
  try {
    const cookie = await chrome.cookies.get({
      url: "https://claude.ai",
      name: "sessionKey",
    });

    if (!cookie || !cookie.value) {
      showNotFound();
      return;
    }

    showFound(cookie.value);
  } catch (err) {
    showError(err.message);
  }
}

function showFound(key) {
  content.innerHTML = `
    <div class="status found">Session key found!</div>
    <div class="key-box">${escapeHtml(key)}</div>
    <button class="copy" id="copyBtn">Copy to Clipboard</button>
    <div class="step" style="margin-top: 12px;">
      <b>Next:</b> Open your device's web portal and paste this key into the Session Key field.
    </div>
  `;

  document.getElementById("copyBtn").addEventListener("click", async () => {
    await navigator.clipboard.writeText(key);
    const btn = document.getElementById("copyBtn");
    btn.textContent = "Copied!";
    btn.className = "copied";
    setTimeout(() => {
      btn.textContent = "Copy to Clipboard";
      btn.className = "copy";
    }, 2000);
  });
}

function showNotFound() {
  content.innerHTML = `
    <div class="status error">No session key found.</div>
    <div class="step">
      <b>1.</b> Go to <a href="https://claude.ai" target="_blank" style="color: #ff9944;">claude.ai</a> and log in<br>
      <b>2.</b> Click this extension again
    </div>
  `;
}

function showError(msg) {
  content.innerHTML = `
    <div class="status error">Error: ${escapeHtml(msg)}</div>
  `;
}

function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str;
  return div.innerHTML;
}

getSessionKey();
