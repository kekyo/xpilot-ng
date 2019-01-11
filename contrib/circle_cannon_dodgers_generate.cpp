#include <iostream>
#include <math.h>

using namespace std;

const float pi = 3.141592653589793238;

const int centerx = 512 * 64;
const int centery = 512 * 64;
const int radius = 400 * 64;
const int vertices = 256;

const int cannonradius = -50 * 64;
const int cannons = 20;

int main()
{
	int cannonx[3];
	int cannony[3];
	cannonx[0] = 747; cannony[0] = -1120;
	cannonx[1] = 747; cannony[1] = 1120;
	cannonx[2] = 0; cannony[2] = 0;
	for (int i=0; i<3; i++) 
		cannonx[i] += cannonradius;

	cout << "<XPilotMap version=\"1.1\">" << endl;
	cout << "<GeneralOptions>" << endl;
	cout << "<Option name=\"afterburnerpowermult\" value=\"1.1\"/>" << endl;
	cout << "<Option name=\"allowplayerbounces\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"allowplayercrashes\" value=\"no\"/>" << endl;
	cout << "<Option name=\"allowplayerkilling\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"allowshields\" value=\"no\"/>" << endl;
	cout << "<Option name=\"allowshipshapes\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"baselesspausing\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"constantspeed\" value=\"0.5\"/>" << endl;
	cout << "<Option name=\"debriswallbounce\" value=\"no\"/>" << endl;
	cout << "<Option name=\"cannonflak\" value=\"false\"/>" << endl;
	cout << "<Option name=\"edgewrap\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"gravity\" value=\"0\"/>" << endl;
	cout << "<Option name=\"initialafterburners\" value=\"2\"/>" << endl;
	cout << "<Option name=\"initialfuel\" value=\"1000\"/>" << endl;
	cout << "<Option name=\"itemprobmult\" value=\"0.0\"/>" << endl;
	cout << "<Option name=\"kineticenergyfueldrain\" value=\"no\"/>" << endl;
	cout << "<Option name=\"limitedlives\" value=\"no\"/>" << endl;
	cout << "<Option name=\"limitedvisibility\" value=\"no\"/>" << endl;
	cout << "<Option name=\"mapauthor\" value=\"\"/>" << endl;
	cout << "<Option name=\"mapname\" value=\"Circle Cannon Dodgers\"/>" << endl;
	cout << "<Option name=\"maxplayershots\" value=\"0\"/>" << endl;
	cout << "<Option name=\"maxshieldedwallbouncespeed\" value=\"100\"/>" << endl;
	cout << "<Option name=\"maxunshieldedwallbouncespeed\" value=\"90\"/>" << endl;
	cout << "<Option name=\"pausedfps\" value=\"0\"/>" << endl;
	cout << "<Option name=\"playersonradar\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"playerstartsshielded\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"playerwallbouncebrakefactor\" value=\"0.5\"/>" << endl;
	cout << "<Option name=\"playerwallfriction\" value=\"0.8\"/>" << endl;
	cout << "<Option name=\"reserverobotteam\" value=\"no\"/>" << endl;
	cout << "<Option name=\"robots\" value=\"0\"/>" << endl;
	cout << "<Option name=\"robotteam\" value=\"0\"/>" << endl;
	cout << "<Option name=\"roundstoplay\" value=\"0\"/>" << endl;
	cout << "<Option name=\"shotswallbounce\" value=\"no\"/>" << endl;
	cout << "<Option name=\"sparkswallbounce\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"teamimmunity\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"teamplay\" value=\"yes\"/>" << endl;
	cout << "<Option name=\"usewreckage\" value=\"no\"/>" << endl;
	cout << "<Option name=\"waitingfps\" value=\"0\"/>" << endl;
	cout << "<Option name=\"wallbouncefueldrainmult\" value=\"0\"/>" << endl;
	cout << "<Option name=\"worldlives\" value=\"0\"/>" << endl;
	cout << "<Option name=\"mapheight\" value=\"1600\"/>" << endl;
	cout << "<Option name=\"mapwidth\" value=\"1600\"/>" << endl;
	cout << "</GeneralOptions>" << endl;
	
	cout << "<Edgestyle id=\"white\" width=\"2\" color=\"FFFFFF\" style=\"0\"/>" << endl;
	cout << "<Polystyle id=\"emptywhite\" color=\"FF\" defedge=\"white\" flags=\"0\"/>" << endl;
	cout << "<Edgestyle id=\"yellow\" width=\"2\" color=\"FFFF00\" style=\"0\"/>" << endl;
	cout << "<Polystyle id=\"emptyyellow\" color=\"FF\" defedge=\"yellow\" flags=\"0\"/>" << endl;

	int x, y;
	int lastx, lasty;
		
	for (int c=0; c<cannons; c++) {
		float a = 2.0*pi / cannons * c;
		
		x = static_cast<int>(cannonradius * cos(a) + centerx);
		y = static_cast<int>(cannonradius * sin(a) + centery);
		
		int cdir = static_cast<int>(64.0 + (a / (2.0*pi) * 128.0)) % 128;
		
		cout << "<Cannon x=\"" << x << "\" y=\"" << y << "\" dir=\"" << cdir << "\">" << endl;
		cout << "<Polygon x=\"" << x << "\" y=\"" << y << "\" style=\"emptywhite\">" << endl;

		lastx = x;
		lasty = y;
		
		for (int i=0; i<3; i++) {
			x = static_cast<int>(cos(a)*cannonx[i] - sin(a)*cannony[i] + centerx);
			y = static_cast<int>(sin(a)*cannonx[i] + cos(a)*cannony[i] + centery);
			cout << "<Offset x=\"" << (x-lastx) << "\" y=\"" << (y-lasty) << "\"/>" << endl;
			lastx = x;
			lasty = y;
		}
			
		cout << "</Polygon></Cannon>" << endl;
	}		

	bool first = true;
	for (int v=0; v<=vertices; v++) {
		float a = -2.0*pi / vertices * v + pi;

		x = static_cast<int>(radius * cos(a) + centerx + 0.5);
		y = static_cast<int>(radius * sin(a) + centery + 0.5);

		if (first) {
			cout << "<Polygon x=\"" << x << "\" y=\"" << y << "\""
				<< " style=\"emptyyellow\">" << endl;
		} else {
			cout << "<Offset x=\"" << (x-lastx) << "\" y=\"" << (y - lasty) << "\"/>" << endl;
		}
		lastx = x;
		lasty = y;
		
		first = false;
	}

	cout << "</Polygon>" << endl;

	cout << "<Base team=\"2\" x=\"" << static_cast<int>(centerx - 350*64) << "\" y=\""
		<< static_cast<int>(centery) << "\" dir = \"64\"/>" << endl;
	cout << "<Base team=\"2\" x=\"" << static_cast<int>(centerx) << "\" y=\""
		<< static_cast<int>(centery + 350*64) << "\" dir = \"32\"/>" << endl;
	cout << "<Base team=\"2\" x=\"" << static_cast<int>(centerx + 350*64) << "\" y=\""
		<< static_cast<int>(centery) << "\" dir = \"0\"/>" << endl;
	cout << "<Base team=\"2\" x=\"" << static_cast<int>(centerx) << "\" y=\""
		<< static_cast<int>(centery - 350*64) << "\" dir = \"96\"/>" << endl;

	cout << "</XPilotMap>" << endl;
	
	return 0;
}
