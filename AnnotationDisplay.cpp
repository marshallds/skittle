#include "AnnotationDisplay.h"
#include "glwidget.h"
#include <sstream>
#include <algorithm>
/** **********************
  This class is a Graph class that is designed to visualize annotation files.
  This is the most unusual of the graphs except for Cylinder.  It makes a slender bar
  on the far right of the screen with colored bars that correspond to each of the entries
  in the annotation.

  Annotations are marked by a beginning an end, and a text string describing what the element is.
  AnnotationDisplay uses a helper class called track_entry that stores this information.
  The beginning and end coordinates are matched up with the nucleotide index in all other views.
  A main issue that the AnnotationDisplay needs to overcome is that annotations can overlap.
  In cases of overlap, it adds another parallel lane and places additional annotation side by side.
  The logic for parallel lanes is in displayTrack().
  **********************/
AnnotationDisplay::AnnotationDisplay(UiVariables* gui, GLWidget* gl, string gtfFileName)
    :AbstractGraph(gui, gl)
{
	max_width = 0;
	
	actionLabel = string("Annotation Display");
	actionTooltip = string("Genome annotation locations");
	actionData = actionLabel; 
	
    fileName = gtfFileName;
}

AnnotationDisplay::~AnnotationDisplay()
{
    glDeleteLists(display_object, 1);
}

bool trackCompare(const track_entry& a, const track_entry& b)
{
	return a.start < b.start;
}

void AnnotationDisplay::newTrack(vector<track_entry> track)
{
	gtfTrack = vector<track_entry>(track);
	sort(gtfTrack.begin(), gtfTrack.end(), trackCompare);
	hidden = false;
	emit displayChanged();
}

void AnnotationDisplay::display()
{
	if( !upToDate )
	{
        glDeleteLists(display_object, 1);
		display_object = render();
	}	
    else
        glCallList(display_object);
}

GLuint AnnotationDisplay::render()
{
    qDebug() << "AnnotationDisplay::render(): " << ++frameCount;
    GLuint Annotation_Display_list = glGenLists(1);
    glNewList(Annotation_Display_list, GL_COMPILE_AND_EXECUTE);
        glPushMatrix();
	
        if( !gtfTrack.empty() )
            displayTrack( gtfTrack );
	
        glPopMatrix();
    glEndList();
    upToDate = true;
    return Annotation_Display_list;
}

void AnnotationDisplay::displayTrack(const vector<track_entry>& track)
{
	max_width = 1;
	int pix_start = ui->startDial->value();
	int width = ui->widthDial->value();
	int pix_stop = pix_start + width;
    int temp_display_size = current_display_size();
    int next_spot = 0;
	
	if(track.empty()) 
		return;
	vector<track_entry> activeEntries = vector<track_entry>();
	track_entry blank = track_entry(0,0, color(0,0,0));
	activeEntries.push_back(blank);
    for(int row = 0; row < temp_display_size / width; row++)
	{
		//check to see if any of the old tracks are done
        for(int k = 0; k < (int)activeEntries.size(); ++k)
		{
			if( !activeEntries[k].isBlank() )
			{
				if( activeEntries[k].stop < pix_start )
				{
					if( activeEntries[k].col == color(200,200,200) )
						activeEntries[k] = blank;//remove the entry
					else
						activeEntries[k].col = color(200,200,200);
				}
			}
		}
		//check to match start for a new track
        while(next_spot < (int)track.size() && track[next_spot].stop < pix_start )//assumes tracks are in order
			next_spot++;
        while(next_spot < (int)track.size()
			&& ((track[next_spot].start >= pix_start && track[next_spot].start <= pix_stop)//start in range
			    || (track[next_spot].stop >= pix_start && track[next_spot].stop <= pix_stop)//end in range
				|| (track[next_spot].start < pix_start && track[next_spot].stop > pix_stop)) )//in the middle
		{
			stackEntry(activeEntries, track[next_spot++]);		//place new tracks in proper position
		}
		
		//display each track
		glPushMatrix();
			glScaled(1,-1,1);
        for(int x = 0; x < (int)activeEntries.size(); ++x)
		{
			color c;
			if(activeEntries[x].isBlank())
                c = color(200,200,200);
			else	
				c = activeEntries[x].col;
				
			glPushName( activeEntries[x].index );
			paint_square( point(x*2,row,0), c );
			paint_square( point(x*2+1,row,0), c );
			glPopName();
			if(x+1 > max_width)
				max_width = x+1;
		}
		glPopMatrix();
		pix_start += width;
		pix_stop += width;
	}
}

void AnnotationDisplay::stackEntry(vector<track_entry>& activeEntries, track_entry item)
{
        for(int k = 0; k < (int)activeEntries.size(); ++k)
		{
			if( activeEntries[k].isBlank() )
			{
				activeEntries[k] = item;
				return;
			}
		}//we reached the end without finding an open spot
		activeEntries.push_back(item);
}

string AnnotationDisplay::mouseClick(point2D pt)
{
	//range check
	if( pt.x <= width() && pt.x >= 0 )
	{
		ui->print("-------------");
		int start = ui->startDial->value() + pt.y * ui->widthDial->value() + pt.x;
		int stop = start + ui->widthDial->value();
		if(!gtfTrack.empty())
		{
            for(int i = 0; i < (int)gtfTrack.size(); ++i)
			{
				if(((gtfTrack[i].start >= start && gtfTrack[i].start <= stop)//start in range
				    || (gtfTrack[i].stop >= start && gtfTrack[i].stop <= stop)//end in range
					|| (gtfTrack[i].start < start && gtfTrack[i].stop > stop)) )//in the middle
				{
					ui->print(gtfTrack[i].toString().c_str());
				}
			}
		}
	}
	return string();
}

int AnnotationDisplay::width()
{
    return max_width*2;
}


string AnnotationDisplay::getFileName()
{
	return fileName;
}

int AnnotationDisplay::current_display_size()
{
    return ui->sizeDial->value();
}

void AnnotationDisplay::setFileName(string gtfFileName)
{
	fileName = gtfFileName;
}

/******SLOTS*****/
void AnnotationDisplay::addEntry(track_entry entry)
{
	gtfTrack.push_back(entry);
	sort(gtfTrack.begin(), gtfTrack.end(), trackCompare);
	emit displayChanged();
}
/**/
