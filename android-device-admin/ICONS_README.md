# Icon Files Placeholder

The mipmap directories contain placeholder SVG files for the launcher icons. In a production build, these should be replaced with actual PNG image files.

## Required PNG Icon Sizes

For a complete Android application, you need to create PNG icons for the following densities:

### Launcher Icons (ic_launcher)
- mdpi: 48x48 pixels
- hdpi: 72x72 pixels
- xhdpi: 96x96 pixels
- xxhdpi: 144x144 pixels
- xxxhdpi: 192x192 pixels

### Round Launcher Icons (ic_launcher_round)
- mdpi: 48x48 pixels
- hdpi: 72x72 pixels
- xhdpi: 96x96 pixels
- xxhdpi: 144x144 pixels
- xxxhdpi: 192x192 pixels

## How to Create Icons

1. Use Android Studio's Image Asset Studio
2. Design using tools like:
   - Adobe Photoshop
   - GIMP
   - Figma
   - Canva
   - Online icon generators

3. Export in PNG format for each required density

## Icon Design Guidelines

- Use the app's brand colors (primary: #6200EE purple)
- Keep it simple and recognizable
- Ensure good contrast
- Follow Material Design guidelines
- Test on different backgrounds

## Replacing Placeholders

Replace the placeholder files in each mipmap density directory with your actual PNG icons:

```
app/src/main/res/
├── mipmap-mdpi/
│   ├── ic_launcher.png
│   └── ic_launcher_round.png
├── mipmap-hdpi/
│   ├── ic_launcher.png
│   └── ic_launcher_round.png
├── mipmap-xhdpi/
│   ├── ic_launcher.png
│   └── ic_launcher_round.png
├── mipmap-xxhdpi/
│   ├── ic_launcher.png
│   └── ic_launcher_round.png
└── mipmap-xxxhdpi/
    ├── ic_launcher.png
    └── ic_launcher_round.png
```

Note: The SVG files in `mipmap-anydpi-v26/` and the drawable foreground can be kept for adaptive icons on Android 8.0+ devices.
