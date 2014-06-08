/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "TwoDReplay.h"

//#include "BeamData.h" // we dont want that here for these consts

#define MAX_TRUCKS 64

const char colors[10][8] = {
		"#FF4848",
		"#5757FF",
		"#1FCB4A",
		"#BABA21",
		"#FF800D",
		"#892EE4",
		"#1F88A7",
		"#990099",
		"#B9264F",
		"#06DCFB"
};

int main(int argc, char**argv)
{
	if(argc != 2)
	{
		printf("usage: %s <file.tdr>\n", argv[0]);
		return 1;
	}
	FILE *file = fopen(argv[1], "r");
	if(!file)
	{
		printf("error opening file: %s\n", argv[1]);
		return 1;
	}

	char fnout[1024]="";
	strncpy(fnout, argv[1], 1024);
	strcat(fnout, ".js");
	FILE *fileo = fopen(fnout, "w");
	if(!fileo)
	{
		printf("error opening ouput file: %s\n", fnout);
		return 1;
	}
	fprintf(fileo, "var data = [\n");

	// first: read and compare header

	r2d_file_header_t header;
	fread(&header, 1, sizeof(header), file);

	if(strncmp(header.fileformat, R2DFILEFORMAT, 8))
	{
		printf("unsupported file format: %s\n", header.fileformat);
		printf("supported fileformat: %s\n", R2DFILEFORMAT);
		return 1;
	}
	printf("loaded file with %d rigs:\n", header.num_rigs);

	int wheel_count[MAX_TRUCKS];

	for(int t=0; t<header.num_rigs; t++)
	{
		r2d_file_wheel_setup_t ws;
		r2d_file_rig_t rig;
		fread(&rig, 1, sizeof(rig), file);
		printf(" %d %s / %s / %s / wheels:%d\n", rig.num, rig.name, rig.filename, rig.hash, rig.num_wheels);
		wheel_count[t] = rig.num_wheels;
		for(int w=0; w < rig.num_wheels; w++)
		{
			fread(&ws, 1, sizeof(ws), file);
			printf("  wheel %d : width: %g, radius:%g, type:%d\n", w, ws.width, ws.radius, ws.type);
		}
	}

	long header_pos = ftell(file);
	// done parsing header, now to the data

	// read all data
	int entries = 0;
	r2d_file_entry_t entry;
	while(!feof(file))
	{
		fread(&entry, 1, sizeof(entry), file);
		entries++;
		printf("got entry: rig:%d, time:%gs, frame:%ld, type:%d\n", entry.rignum, (float)entry.time_ms/1000.0f, entry.physframe, entry.type);
		fprintf(fileo, "\t{ ");
		fprintf(fileo, "type: %d, ", entry.type);
		fprintf(fileo, "rignum: %d, ", entry.rignum);
		fprintf(fileo, "time_ms: %ld, ", entry.time_ms);
		fprintf(fileo, "physframe: %ld, ", entry.physframe);
		fprintf(fileo, "\n");
		

		r2d_memory_structure_rig_t st;
		st.entry = entry;

		if(entry.type == R2DTYPE_FRAME)
		{
			r2d_file_frame_t frame;
			fread(&frame, 1, sizeof(frame), file);
			fprintf(fileo, "\t\tframe: {");
			fprintf(fileo, "brake: %g, ", frame.brake);
			fprintf(fileo, "steering: %g, ", frame.steering);
			fprintf(fileo, "accel: %g, ", frame.accel);
			fprintf(fileo, "clutch: %g, ", frame.clutch);
			fprintf(fileo, "gear: %d, ", frame.gear);
			fprintf(fileo, "gearMode: %d, ", frame.gearMode);
			fprintf(fileo, "pbrake: %d, \n", frame.pbrake);
			fprintf(fileo, "\t\t\twheels: { \n");

			r2d_file_wheel_frame_t wf;
			for(int w=0; w < wheel_count[entry.rignum]; w++)
			{
				fread(&wf, 1, sizeof(wf), file);
				fprintf(fileo, "\t\t\t\t%d: { ", w);
				fprintf(fileo, "x: %g, ", wf.wheel_pos[0]);
				fprintf(fileo, "y: %g, ", wf.wheel_pos[1]);
				fprintf(fileo, "z: %g, ", wf.wheel_pos[2]);
				fprintf(fileo, "dir: %g, ", wf.wheel_direction);
				fprintf(fileo, "slip: %g, ", wf.wheel_slip);
				fprintf(fileo, "speed: %g", wf.wheel_speed);
				fprintf(fileo, " },\n");
			}
			fprintf(fileo, "\t\t\t},\n");
			fprintf(fileo, "\t\t},\n");
		}
		fprintf(fileo, "\t}, \n");
	}
	fclose(file);
	fprintf(fileo, "];\n");
	fprintf(fileo, "var data_size = %d\n", entries);
	fclose(fileo);
	printf("done, saved as file %s\n", fnout);
	return 0;
}
