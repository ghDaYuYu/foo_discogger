# foo_discogs

foo_discogs is a highly-customizable audio file tagging component for the [foobar2000](http://www.foobar2000.org/) music player. Using this component, you can quickly and easily tag your music files with information pulled from the [Discogs](http://www.discogs.com) database.

# Getting Started

## Download

Get the component from [the official foobar2000 components page](http://www.foobar2000.org/components/view/foo_discogs).

# Basic Usage

How to tag your music:

### 1. Select the tracks in your foobar2000 playlist that you'd like to tag.

Right click. Select Tagging > Discogs > Write Tags... (see below for hotkeys)

### 2. Use the Find Release dialog to find the exact release you will to pull meta-data from.

The plugin will automatically try to search for the artist name and filter for the release name:

![foo_discogs_write_tags1.png](https://bitbucket.org/repo/qo984z/images/2837649743-foo_discogs_write_tags1.png)

Sometimes, you'll need to manually edit the artist name to get results:

![foo_discogs_write_tags2.png](https://bitbucket.org/repo/qo984z/images/2190118245-foo_discogs_write_tags2.png)

If your release has multiple versions on Discogs, select the correct version under the master release entry:

![foo_discogs_write_tags3.png](https://bitbucket.org/repo/qo984z/images/41404439-foo_discogs_write_tags3.png)

Press "Next" to continue to the Release dialog.

### 3. Use the Release dialog to write meta-data for your tracks.
The Release dialog shows a synopsis of information about the release including, optionally, a thumbnail of it:

![foo_discogs_write_tags4.png](https://bitbucket.org/repo/qo984z/images/3920796611-foo_discogs_write_tags4.png)

Usually you can just press "Write Tags" at this point. Your audio files have now been tagged!

Occasionally, the tracks from the Discogs (on the left side) and the tracks you've selected (on the right side) might not match correctly. This is a sign that something is probably wrong; you may need to reorder or remove some tracks or go back and select a different release.

## Other Usage

Besides tagging, foo_discogs offers some other functions such as:

* Automatically updating your tags.
* Opening the Discogs page associated with a release, it's artist, or label in your browser.

# OAuth

Discogs.com now requires OAuth authentication for anyone to access its API. To authenticate via OAuth, you'll need to have a Discogs account. When you first open the Find Release dialog without having configured OAuth, you will automatically be prompted to do so. Follow the instructions on the OAuth Identity tab.

# Configuration

Press "Configure" on the Find Release dialog or find it in the context menu under Tagging > Discogs to open the Configuration dialog. On the Tagging tab, you'll find some basic options regarding how foo_discogs tags your files. On the OAuth Identity tab, you can configure OAuth if you haven't already. The Album/artist Art tab is described in more detail below.

## Album/Artist Art (Images)

If you'd like to download Album and/or Artist Art (images), open the second tab on the Configuration dialog to set it up. 

![foo_discogs_config_images.png](https://bitbucket.org/repo/qo984z/images/3171312933-foo_discogs_config_images.png)

Album art is saved as cover.jpg by default, if fetching is enabled. If you wish to download all images rather than just the first one, check "Fetch all available art".

NOTE: Discogs API currently has a limit of 1000 images per application (ie. for all foo_discogs users) via the API. This would mean that each foo_discogs user could access less than 2 images per day (or a single user could download 1000 and everyone else none), and that would result in a lot of unhappy users. Therefore, the default setting of foo_discogs is to bypass the API for images. This is a violation of Discogs' terms, so I recommend not abusing image downloading as Discogs has been known to ban users for it. Hopefully they'll fix the limit on their API to be per user instead of per application so nobody will need to bypass the API for images any longer.

## Tag Mappings

Advanced users may wish to edit the tags that are written to their music files. Use the "Edit tag mappings" dialog to configure which tags are written and what name they're written to.

![foo_discogs_edit_tag_mappings.png](https://bitbucket.org/repo/qo984z/images/3906017670-foo_discogs_edit_tag_mappings.png)

## Keyboard Shortcuts

If you tag music frequently, you'll probably want to configure keyboard shortcuts for foo_discogs within foobar2000. To do so, open foobar2000's preferences and select Keyboard Shortcuts.

Press "Add new" and filter the Actions looking for the Discogs context:

![foo_discogs_hotkeys1.png](https://bitbucket.org/repo/qo984z/images/4173510150-foo_discogs_hotkeys1.png)

Select whichever Action you're interested in ("Write Tags" to open the Find Release dialog for the selected tracks). In the Key box, type whatever hotkey you'd like to set for the action:

![foo_discogs_hotkeys2.png](https://bitbucket.org/repo/qo984z/images/3916785501-foo_discogs_hotkeys2.png)



# About

## Authors

* Michael Pujos (aka bubbleguuum) (up until v1.32)
* zoomorph (v1.33+)

## Community

Check out the age-old thread about foo_discogs on [hydrogenaudio forums](http://www.hydrogenaud.io/forums/index.php?showtopic=50523).

## Bug or Feature Request?

File it on the Issues tracker on BitBucket.

## Development

foo_discogs is built with Microsoft Visual Studio. Feel free to get your hands dirty, add a feature, and submit a pull request.

### Dependencies

* foobar2000 SDK - http://www.foobar2000.org/SDK
* Jansson (JSON parser) - http://www.digip.org/jansson/
* liboauthcpp (OAuth library) - https://github.com/sirikata/liboauthcpp
* zlib (Compression library) - http://www.zlib.net/