// Copyright (c) 2007-2009 Google Inc.
// Copyright (c) 2006-2007 Jaiku Ltd.
// Copyright (c) 2002-2006 Mika Raento and Renaud Petit
//
// This software is licensed at your choice under either 1 or 2 below.
//
// 1. MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// 2. Gnu General Public license 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
// This file is part of the JaikuEngine mobile client.

#include <stdio.h>
#include <stdlib.h>
//#include <fstream.h>

#include "riff.h"
#include "gain_analysis.h"

#define MAX_CHUNK 10000

const int MAX_CHANNELS = 4;   // make larger if needed

/*
void showVersion() {
	cerr << "WavGain version 0.1" << endl;
}

void showUsage(const char *progname) {
	showVersion();
	cerr << endl << "Usage: " << progname << " [options] <filename> [<filename 2> ...]" << endl;
	cerr << "  --use \"" << progname << " /?\" for a full list of options" << endl << endl;
	cerr << "each line of output is \"<filename> [TAB] <radio gain> [TAB] <peak (ratio)>\"" << endl;
	cerr << "last line is \"Album [TAB] <album gain> [TAB] <peak (ratio)>\"" << endl;
}

void showFullUsage(const char *progname) {
	showVersion();
	cerr << endl << "Usage: " << progname << " [options] <filename> [<filename 2> ...]" << endl << endl;
	cerr << "each line of output is \"<filename> [TAB] <radio gain> [TAB] <peak (ratio)>\"" << endl;
	cerr << "last line is \"Album [TAB] <album gain> [TAB] <peak (ratio)>\"" << endl << endl;
	cerr << "Options: " << endl;
	cerr << "\t/v - show program version number" << endl;
	cerr << "\t/? - show options" << endl;
	cerr << "\t...ummm... that's about all the options for now." << endl;
}
*/

/*
int doFile(const char *inFilename, unsigned long *lastFreq, long *maxSample) {
	long maxFileSample = 0;
	long totsamp = 0;

	GainAnalysis ga;

	ga.InitGainAnalysis(*lastFreq);

	WaveFile inWave;
	DDCRET rc = inWave.OpenForRead( inFilename );

	switch (rc) {
	case DDC_SUCCESS:           // The operation succeded
		break;

	case DDC_INVALID_FILE:       // File format does not match
		//TODO
		//cerr << inFilename << " is not a valid WAV file" << endl;
		return 0;

	default:           // The operation failed for unspecified reasons
		//TODO
		//cerr << "Error opening WAV file " << inFilename << endl;
		return 0;

	}

	int bit8;

	switch (inWave.BitsPerSample()) {
		case 8:
			bit8 = 1;
			break;

		case 16:
			bit8 = 0;
			break;

		default:
			//TODO
			//cerr << "Sorry, only 8-bit or 16-bit WAV files" << endl;
			return 0;
	}

	if (inWave.SamplingRate() != *lastFreq) {
		*lastFreq = inWave.SamplingRate();
		ga.ResetSampleFrequency(*lastFreq);
	}

	if (inWave.NumChannels() == 2) {
		Float_t lsamps[MAX_CHUNK], rsamps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 lsamp, rsamp;

		rc = inWave.ReadStereoSample(&lsamp,&rsamp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {

				if (bit8) {
					lsamp = ((lsamp - 128) << 8) - 1;
					rsamp = ((rsamp - 128) << 8) - 1;
				}

				if (lsamp > maxFileSample)
					maxFileSample = lsamp;
				else if (-lsamp > maxFileSample)
					maxFileSample = -lsamp;

				if (rsamp > maxFileSample)
					maxFileSample = rsamp;
				else if (-rsamp > maxFileSample)
					maxFileSample = -rsamp;

				lsamps[cursamp] = (Float_t)(lsamp); // << multiplier);
				rsamps[cursamp++] = (Float_t)(rsamp); // << multiplier);

				rc = inWave.ReadStereoSample(&lsamp,&rsamp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				ga.AnalyzeSamples(lsamps,rsamps,cursamp,2);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else if (inWave.NumChannels() == 1) {
		Float_t samps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 samp;

		rc = inWave.ReadMonoSample(&samp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {
				
				if (bit8) {
					samp = ((samp - 128) << 8) - 1;
				}

				if (samp > maxFileSample)
					maxFileSample = samp;
				else if (-samp > maxFileSample)
					maxFileSample = -samp;

				samps[cursamp++] = (Float_t)(samp); // << multiplier);

				rc = inWave.ReadMonoSample(&samp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				ga.AnalyzeSamples(samps,NULL,cursamp,1);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else {
		//TODO
		//cerr << "Can't handle more than 2 channels." << endl;
		return 0;
	}

	if (maxFileSample > *maxSample)
		*maxSample = maxFileSample;

	inWave.Close();

	if (totsamp > 0) {
		//TODO
		//cout << inFilename << "\t" << GetTitleGain() << "\t" << ((double)(maxFileSample) / 32767.0) << endl;
		return 1;
	}

	return 0;
}
*/

/*
int main ( int argc, const char *argv[] )
{
	if (argc < 2) {
		showUsage(argv[0]);
		return 1;
	}

	unsigned long lastFreq = 44100;
	int fileStart = 1;
	int okFilecount = 0;
	long maxSample = 0;
	int i;

	InitGainAnalysis(lastFreq);

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '/') {
			fileStart++;
			switch(argv[i][1]) {
				case 'h':
				case 'H':
				case '?':
					showFullUsage(argv[0]);
					return 1;

				case 'v':
				case 'V':
					showVersion();
					return 1;

				default:
					cerr << "Unrecognized option: /" << argv[i][1] << endl;
			}
		}
	}

	for (i = fileStart; i < argc; i++) {
		okFilecount += doFile(argv[i], &lastFreq, &maxSample);
	}

	if (okFilecount)
		cout << "Album" << "\t" << GetAlbumGain() << "\t" << ((double)(maxSample) / 32767.0) << endl;

    return 0;
}

*/
