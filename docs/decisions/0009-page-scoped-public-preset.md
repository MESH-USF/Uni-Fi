# 0009 — Page-scoped preset Public chat and location advert

Date: 2026-10-08. Status: accepted; updates decision 0008's button action.

The user reports the location advert queues but is not visible in the app.
They request a preset Public-channel message and a separate location page
listening for triple tap. We have not independently established the cause of
the missing advertisement: sender-only observation, app discovery settings,
or RF delivery are still possible.

Version 0.3.1 uses the home page for preset Public chat and places a location
advert page immediately next. Triple press is handled by the selected page;
other settings and message-preview pages never transmit. A sleeping screen
wakes without changing the selected action. Single/double navigation stays.

The preset includes the saved-name prefix, a test check-in, and explicitly
simulated coordinates. It uses the standard group sender, with no custom wire
format. Match Public by its actual known key, not assumed slot zero or a label.
Reuse the decoder in BaseChatMesh's existing compilation unit to avoid the
Base64 header's multiple-definition linker issue. No silent channel overwrite.

Do not mirror outgoing hardware sends as fake incoming messages to the
sender's app. Verify Public reception on a second node; local acceptance is
not delivery. Keep the original signed public location advert on its own page.
No new GPS framework, beacon timers, app fork or persistent mode state.
