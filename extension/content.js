// Content script: detects the ClaudeGauge config page
// and injects an auto-fill button for the session key.
// Only activates when <meta name="claudegauge"> is present.

(function () {
  const marker = document.querySelector('meta[name="claudegauge"]');
  if (!marker) return;

  const input = document.getElementById("sessionkey");
  if (!input) return;

  // Find the hint div after the session key instructions
  const instructions = input.closest(".section").querySelector(
    'div[style*="background:#0d1117"]'
  );

  // Create the auto-fill button
  const btn = document.createElement("button");
  btn.type = "button";
  btn.textContent = "Auto-fill from Claude.ai";
  Object.assign(btn.style, {
    width: "100%",
    padding: "10px",
    marginBottom: "14px",
    background: "#ff9944",
    color: "#0d1117",
    border: "none",
    borderRadius: "6px",
    fontSize: "14px",
    fontWeight: "600",
    cursor: "pointer",
  });

  btn.addEventListener("click", () => {
    btn.disabled = true;
    btn.textContent = "Reading session key...";
    btn.style.opacity = "0.7";

    chrome.runtime.sendMessage({ type: "getSessionKey" }, (response) => {
      if (chrome.runtime.lastError) {
        showError("Extension error. Try reloading the page.");
        return;
      }

      if (response && response.key) {
        input.value = response.key;
        input.type = "text";
        btn.textContent = "Session key filled!";
        btn.style.background = "#00ff88";
        btn.style.color = "#0d1117";

        // Hide the manual instructions since they're not needed
        if (instructions) instructions.style.display = "none";
      } else {
        showError("Not logged in. Open claude.ai, log in, then try again.");
      }
    });
  });

  function showError(msg) {
    btn.textContent = msg;
    btn.style.background = "#f85149";
    btn.style.color = "#fff";
    btn.disabled = false;
    setTimeout(() => {
      btn.textContent = "Auto-fill from Claude.ai";
      btn.style.background = "#ff9944";
      btn.style.color = "#0d1117";
    }, 3000);
  }

  // Insert button before the manual instructions
  if (instructions) {
    instructions.parentNode.insertBefore(btn, instructions);
  } else {
    input.parentNode.insertBefore(btn, input.nextSibling);
  }
})();
