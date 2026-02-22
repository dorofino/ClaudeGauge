# Privacy Policy — Claude Session Key Helper

**Last updated:** February 27, 2026

## Overview

Claude Session Key Helper is a browser extension that helps users configure their ClaudeGauge device by reading the Claude.ai session key cookie from the browser.

## What data we access

This extension accesses **only one piece of data**:
- The `sessionKey` cookie from the `claude.ai` domain

## How we use the data

The session key is used exclusively to:
- Display it in the extension popup so you can copy it
- Auto-fill the session key field on your ClaudeGauge device's local configuration page (served over HTTP on your local network)

## What we DO NOT do

- We do **not** collect or store any personal data
- We do **not** transmit data to any external servers
- We do **not** track usage or analytics
- We do **not** access any cookies other than `sessionKey` from `claude.ai`
- We do **not** modify any cookies or website content on claude.ai
- We do **not** access browsing history, bookmarks, or any other browser data

## Data storage

This extension does not store any data persistently. The session key is held in memory only while the popup is open or while auto-filling the device configuration form. No data is written to local storage, sync storage, or any database.

## Third-party sharing

We do not share any data with third parties. The extension operates entirely between your browser and your local device.

## Content script

The extension injects a content script on HTTP pages to detect ClaudeGauge device configuration pages (identified by a specific meta tag). The content script only activates on pages that contain `<meta name="claudegauge">` and does nothing on all other pages.

## Permissions explained

- **cookies**: Required to read the `sessionKey` cookie from claude.ai
- **host_permissions (https://claude.ai/*)**: Required to access cookies from the claude.ai domain
- **content_scripts (http://\*/\*)**: Required to detect and auto-fill the device's local configuration page, which runs on HTTP on the local network

## Contact

For questions about this privacy policy, please open an issue at:
https://github.com/dorofino/claudegauge

## Changes

Any changes to this privacy policy will be reflected by updating the "Last updated" date above.
