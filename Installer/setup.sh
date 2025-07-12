#!/bin/bash
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo 'root privileges required' >&2
    exit 1
fi

copy_sl_config() {
	if [ -d "/opt/SM5-LTS/Themes/Simply Love.old/$1" ]; then
		# Don't overwrite any existing files.
		# This lets us retain the updated readmes and other files that come with the new release.
		cp -r -n "/opt/SM5-LTS/Themes/Simply Love.old/$1" "/opt/SM5-LTS/Themes/Simply Love/$1"
		return
	elif [ -f "/opt/SM5-LTS/Themes/Simply Love.old/$1" ]; then
		cp "/opt/SM5-LTS/Themes/Simply Love.old/$1" "/opt/SM5-LTS/Themes/Simply Love/$1"
	fi
}

cd "$(dirname "$0")"

# Move the old SL release out of the way
[ -d /opt/SM5-LTS/Themes/Simply\ Love ] && mv /opt/SM5-LTS/Themes/Simply\ Love{,.old}

# Install ITGm
[ -d /opt ] || install -d -m 755 -o root -g root /opt
cp -R --preserve=mode,timestamps SM5-LTS /opt
ln -sf /opt/SM5-LTS/SM5-LTS.desktop /usr/share/applications

# Copy persistent files over from the old SL folder
if [ -d /opt/SM5-LTS/Themes/Simply\ Love.old ]; then
	copy_sl_config Other/SongManager\ PreferredCourses.txt
	copy_sl_config Other/SongManager\ PreferredSongs.txt
	copy_sl_config Modules

	rm -rf /opt/SM5-LTS/Themes/Simply\ Love.old
fi
