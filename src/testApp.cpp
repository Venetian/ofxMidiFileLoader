


#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	//midiFileName = "../../../data/entertainer.mid";
//	midiFileName = "/Users/andrew/Music/Logic/GreenOnionsChichester/GreenOnionsChichester/Bouncing/GreenOnionsMain.mid";
	midiFileName = "/Users/andrew/Music/Logic/GreenOnionsChichester/GreenOnionsChichester/Bouncing/main2_redone.mid";
	loader.loadFile(midiFileName);
	loader.printNoteData();
	
	
	//loader.filterMidiEvents();
	//loader.printNoteData();
	
/*
	int test = 43*128*128;
	printf("test %i\n", test);
	test += 44*128;
	printf("test %i\n", test);
 */
}

//--------------------------------------------------------------
void testApp::update(){
}

//--------------------------------------------------------------
void testApp::draw(){
	
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    
	if (key == 'o'){
		openMidiFile();
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}



void testApp::openMidiFile(){
	//choose file with ofxFileDialog
	string *filePtr;
	filePtr = &midiFileName;
	
	if (getFilenameFromDialogBox(filePtr)){
		printf("Midifile: Loaded name okay :\n'%s' \n", midiFileName.c_str());
		loader.loadFile(midiFileName);
	}
	
}

bool testApp::getFilenameFromDialogBox(string* fileNameToSave){
	std::string URL;
	
	ofFileDialogResult fileResult = ofSystemLoadDialog("Choose MIDI file to load");
    
	if(fileResult.bSuccess){
		// now you can use the URL
		*fileNameToSave = fileResult.filePath;
		//printf("\n filename is %s \n", soundFileName.c_str());
		return true;
	}
	else {
		printf("open file cancelled \n");
		return false;
	}

}