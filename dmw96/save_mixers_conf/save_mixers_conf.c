/*
 *   ALSA command line mixer configuration saving utility
 *   Copyright (c) 2012 by Avi Miller <avi.miller@dspg.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

static const char s_cntrl[] = "AndroidOut";

static const char s_prefix[] = "\t\t\t{ name  ";
static const char s_PVolume[] = " Playback Volume"; 
static const char s_CVolume[] = " Capture Volume"; 
static const char s_value[]  = "\t\t\tvalue";
static const char s_suffix[] = "\t\t}";	

static void error(const char *fmt,...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "dspg-audio-conf: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

static int filter_out_elem(const char * name)
{
	if ( (strstr(name, "ALC"))	||
	     (strstr(name, "HPass"))	||
	     (strstr(name, "Limiter"))	||
	     (strstr(name, "BEEP"))	||
	     (strstr(name, "EQ")) ) {

		/* Filter out */
		return 1;
	}
	return 0;
}

static int filter_in_enum_playback_elem(const char * name)
{
	if (strstr(name, "PwrAmp-Gain")) {

		/* Filter in */
		return 1;
	}
	return 0;
}


static int filter_in_enum_capture_elem(const char * name)
{
	if (strstr(name, "Input-PGA-Boost")) {

		/* Filter in */
		return 1;
	}
	return 0;
}


static int show_volume_selem(snd_mixer_t *handle,
			     snd_mixer_selem_id_t *id,
			     const char *dir,
			     FILE * file)
{
	snd_mixer_elem_t *elem;
	const char * name;
	long pVol0, pVol1, cVol0, cVol1;
	int pmono, cmono;
	unsigned int idx;
	char itemname[64];

	if ( (strcmp(dir, "Playback") != 0) &&
	     (strcmp(dir, "Capture")  != 0) ) {
		return 0;
	}
	
	elem = snd_mixer_find_selem(handle, id);
	if (!elem) {
		error("Mixer %s simple element not found", s_cntrl);
		return -ENOENT;
	}

	/* Filter out controls */
	name = snd_mixer_selem_id_get_name(id);
	if (filter_out_elem(name)) {
		return 0;
	}

	if (strcmp(dir, "Playback") == 0) {

		/* volume control */
		if (snd_mixer_selem_has_playback_volume(elem)) {
			pmono = snd_mixer_selem_is_playback_mono(elem) ? 1 : 0;
			if (pmono) {
				/* mono */
				snd_mixer_selem_get_playback_volume(elem,
						SND_MIXER_SCHN_MONO, &pVol0);
				fprintf(file, "%s'%s%s'%s\t%li%s\n",
					s_prefix, name, s_PVolume,
					s_value, pVol0, s_suffix);
			} else {
				/* assuming stereo */
				snd_mixer_selem_get_playback_volume(elem,
						SND_MIXER_SCHN_FRONT_LEFT,  &pVol0);
				snd_mixer_selem_get_playback_volume(elem,
						SND_MIXER_SCHN_FRONT_RIGHT, &pVol1);

				fprintf(file, "%s'%s%s'%s.0\t%li%s\n",
					s_prefix, name, s_PVolume,
					s_value, pVol0, s_suffix);
				fprintf(file, "%s'%s%s'%s.1\t%li%s\n",
					s_prefix, name, s_PVolume,
					s_value, pVol1, s_suffix);
			}
		}

		/* enum control */
		else if (snd_mixer_selem_is_enumerated(elem)) {
			if (filter_in_enum_playback_elem(name)) {
				if (snd_mixer_selem_get_enum_item(elem, 0, &idx) == 0) {
					snd_mixer_selem_get_enum_item_name(
						elem, idx,
						sizeof(itemname) - 1, itemname);
					
					fprintf(file, "%s'%s'%s\t'%s'%s\n",
						s_prefix, name, s_value,
						itemname, s_suffix);
				}
			}
		}
	} 

	else if (strcmp(dir, "Capture") == 0) {
		
		/* volume control */
		if (snd_mixer_selem_has_capture_volume(elem)) {
			cmono = (snd_mixer_selem_is_capture_mono(elem)) ? 1 : 0;
			if (cmono) {
				/* mono */
				snd_mixer_selem_get_capture_volume(elem,
						SND_MIXER_SCHN_MONO, &cVol0);
				fprintf(file, "%s'%s%s'%s\t%li%s\n",
					s_prefix, name, s_CVolume,
					s_value, cVol0, s_suffix);
			} else {
				/* assuming stereo */
				snd_mixer_selem_get_capture_volume(elem,
						SND_MIXER_SCHN_FRONT_LEFT,  &cVol0);
				snd_mixer_selem_get_capture_volume(elem,
						SND_MIXER_SCHN_FRONT_RIGHT, &cVol1);

				fprintf(file, "%s'%s%s'%s.0\t%li%s\n",
					s_prefix, name, s_CVolume,
					s_value, cVol0, s_suffix);
				fprintf(file, "%s'%s%s'%s.1\t%li%s\n",
					s_prefix, name, s_CVolume,
					s_value, cVol1, s_suffix);

			}
		}

		/* enum control */
		else if (snd_mixer_selem_is_enumerated(elem)) {
			if (filter_in_enum_capture_elem(name)) {
				if (snd_mixer_selem_get_enum_item(elem, 0, &idx) == 0) {
					snd_mixer_selem_get_enum_item_name(
						elem, idx,
						sizeof(itemname) - 1, itemname);
					
					fprintf(file, "%s'%s'%s\t'%s'%s\n",
						s_prefix, name, s_value,
						itemname, s_suffix);
				}
			}
		}
	}
	
	return 0;
}


static int selems(const char * direction, const char * filename)
{
	int err;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	FILE * file;

	snd_mixer_selem_id_alloca(&sid);
	
	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		error("Mixer %s open error: %s", s_cntrl, snd_strerror(err));
		return err;
	}
	
	if ((err = snd_mixer_attach(handle, s_cntrl)) < 0) {
		error("Mixer attach %s error: %s", s_cntrl, snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	
	if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
		error("Mixer register error: %s", snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	
	err = snd_mixer_load(handle);
	if (err < 0) {
		error("Mixer %s load error: %s", s_cntrl, snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}

	file = fopen(filename, "w");
	if (file != NULL) {
		/* Save specific ASoC controls in file */
		for (elem = snd_mixer_first_elem(handle);
		     elem != NULL;
		     elem = snd_mixer_elem_next(elem)) {
			snd_mixer_selem_get_id(elem, sid);
			show_volume_selem(handle, sid, direction, file);
		}
		fclose(file);
	}
	else {
		printf("Failed to open file %s\n", filename);
	}
	

	snd_mixer_close(handle);
	return 0;
}

const char * s_usage =
	"usage: save_mixer_conf  P  S|E|H  C|M\n"
	"\tP - Playback direction\n"
	"\t      S - Speaker, E - Earpiece, H - Headset\n"
	"\t      C - In Call, M - Not in Call: music, ring-tone, voice-prompt, etc\n"
	" or:\n"
	"       save_mixer_conf  C  B|H\n"
	"\tC - Capture direction\n"
	"\t      B - onBoard mic, H - Headset mic\n"
	"\n"
	" Examples:\n"
	" 1. for Music Playback on Speaker:    save_mixer_conf P S M\n"
	" 2. for Call Playback on Earpiece:    save_mixer_conf P E C\n"
	" 3. for Capture using on-Board mic:   save_mixer_conf C B\n"
	" 4. for Capture using Headset mic:    save_mixer_conf C H\n";

int main(int argc, char *argv[])
{
	const char * direction = NULL;
	const char * out_dev = NULL;
	const char * is_call = NULL;
	const char * mic_type = NULL;
	char filename[80];

	if (argc > 1) {
		if (strcmp(argv[1], "P") == 0) {

			/* Playback */

			if (argc < 4) {
				goto L_EXIT;
			}
			direction = "Playback";

			if (strcmp(argv[2], "S") == 0) {
				out_dev	= "Speaker";
			}
			else if (strcmp(argv[2], "E") == 0) {
				out_dev = "Earpiece";
			}
			else if (strcmp(argv[2], "H") == 0) {
				out_dev = "Headset";
			}
			else {
				goto L_EXIT;
			}

			if (strcmp(argv[3], "C") == 0) {
				is_call = "incall";
			}
			else if (strcmp(argv[3], "M") == 0) {
				is_call = "";
			}
			else {
				goto L_EXIT;
			}
		}

		else if (strcmp(argv[1], "C") == 0) {
			
			/* Capture */

			if (argc < 3) {
				goto L_EXIT;
			}
			direction = "Capture";

			if (strcmp(argv[2], "B") == 0) {
				mic_type = "OnBoard";
			}
			else  if (strcmp(argv[2], "H") == 0) {
				mic_type = "Headset";
			}
			else {
				goto L_EXIT;
			}
		}

		else {
			goto L_EXIT;
		}

	}
	
	else {
		goto L_EXIT;
	}


	if (strcmp(direction, "Playback") == 0) {
		sprintf(filename, "/etc/Android%s_%s", direction, out_dev);
		if (strcmp(is_call, "") != 0) {
			strcat(filename, "_");
			strcat(filename, is_call);
		}
		strcat(filename, "_gains.conf");
	}
	
	else if (strcmp(direction, "Capture") == 0) {
			sprintf(filename, "/etc/Android%s", direction);
			strcat(filename, "_");
			strcat(filename, mic_type);
			strcat(filename, "_gains.conf");
		}

	printf("Saving %s\n", filename);

	selems(direction, filename);
	
	return 0;
	
L_EXIT:
	printf("%s\n", s_usage);
	return 0;
}

