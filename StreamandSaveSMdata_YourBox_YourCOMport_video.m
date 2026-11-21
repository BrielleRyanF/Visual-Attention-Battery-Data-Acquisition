%--------------------------------------------------------------------------
%StreamandSaveSMdata_YourBox_YourCOMPort_video
%By: Brielle Ferguson
%Date: 1/10/18
%Updated: 6/15/24
%Script will start, stream, and save serial monitor data generated from the arduino
%"OperantTaskMasterScript" sketch along with video during a behavioral session. The OperantTaskMasterScript
% sketch must be uploaded first through the Arduino IDE before running this 
% script in MATLAB.
%Assumptions:
% - Arduino sketch is already loaded
% - cd is set to the folder for saving data
% - path includes all helper functions for readAnalyzePlotBehData
% - user is not accessing camera from any other software, will cause an error
%Inputs: 
% - animalID and sessionType collected from input box upon start
%Outputs: none, following variables are automatically saved in cd
% - behData: a cell array with trial information about the behavioral
% - session. 
% - baseFileName: animal ID the user saved when starting session
% - behStartTime: beginning of behavior session 
% - vidStartTime: beginning of video 
%--------------------------------------------------------------------------

%% define session parameters
clearvars
close all
serialPort = 'COM7'; %the port the arduino is connected to
behaviorBoxID = 'Box 3B - COM 7'; %name your box here
copyToProjectFolder = true; %will copy all data to a separate folder at end of session
sharedDriveStartPath = 'G:\Shared drives\FergusonLab\Data\Shared\Projects';
optoStim = false;
automateSessionChange = false;

%% video parameters
sessionLength = minutes(120); %enter session length in minutes
recordVideo = true; %record vide to current directory
streamVideo = true; %if recordVideo is set to false, and this set to true, will stream video of session. Set to false if accessing camera from other software
myAdaptorID = 1; %Must find and define for each camera. Currently for Rack 1 - 3, Rack 3 - 1 ('win') for all USB cameras
myCameraID = 2; %Currently are -  Box 3A - 1, Box 3B - 2, SI Box  - N/A; 
inputFormatID = 8; %The desired video resolution. Using MJPG320x240
frameRate = '15.0000';
directoryToSaveTo = cd;
vidFileName = 'vid.avi';

%% set up serial port 
delete(instrfindall); %make sure no serial ports are already open
baudeRate = 115200; %make sure this matches the rate in the arduino script that you're streaming your serial monitor data
s = serial(serialPort,'Timeout', 5000, 'BaudRate', baudeRate, 'Terminator', 'LF');
s.ReadAsyncMode = 'continuous';
fopen(s);
behStartTime = datetime(now,'ConvertFrom','datenum', 'Format', 'HH:mm:ss.SSS');

%% start video recording
imaqreset
if recordVideo == true
%start video
[vid, vidStartTime] = startUSBCamera(myAdaptorID,myCameraID,inputFormatID,frameRate);

%set up logger to save video
diskLogger = VideoWriter(vidFileName, 'Motion JPEG AVI');
diskLogger.FrameRate = str2num(frameRate);
diskLogger.Quality = 20; 
open(diskLogger);
totalFrames = getdata(vid, vid.FramesAvailable); %grab the first few frames so there is a video array that can be added to as the behavior session runs 
end

%% pre-allocate variables to be populated in the loop
timeStamp = {};
formatOut = 'hh:mm:ss.fff';
behData = {};
index = 1;
running = true;

%% create figure asking user for filename and then indicating that the session is running

startupFcn(queryUser_FilenameAndSessionType);
load results
baseFileName = results.baseFileName;
sessionType = results.sessionType;
sessionDay = results.sessionDay(end);
delete results

if automateSessionChange
% send session type and day to arduino
fprintf(s,'%s\n', sessionType); 
fprintf(s,'%s\n', sessionDay); 
end

if exist(fullfile(cd,'behData.mat'),'file')
    warning(['behData.mat already exists and will be overwritten in this session. Close window to end session and not save'])
end

%figure indicating that MATLAB is collecting Arduino data. User can click end to
%end the session 

if ~recordVideo %create video object so can preview video without recording
if streamVideo
    vid = previewUSBCamera(myAdaptorID,myCameraID,inputFormatID,false);
    behaviorBoxID = strcat(behaviorBoxID, ' - not saving video');
end
end

fig2 = figure('Name', behaviorBoxID); 

if recordVideo || streamVideo
vidRes = vid.VideoResolution; 
nBands = vid.NumberOfBands; 
hImage = image( zeros(vidRes(2), vidRes(1), nBands) ); 
preview(vid, hImage); 
end

h4 = uicontrol(fig2,...
    'Style','text',...
    'String', 'Collecting Serial Monitor Data...',...
    'Position', [150 390 300 20], 'FontSize', 10);
h5 = uicontrol(fig2, 'Style', 'pushbutton', 'String', 'Save and End Session', 'Position', [450 5 117 20],'Callback', 'delete(gcbf);');

%% display and save serial monitor data until the user ends the session         
while true
    drawnow
    
    if  (s.BytesAvailable > 0)
        %read event and time information from the arduino
        event = fgetl(s); 
        timeString = char(datetime('now','Format', 'HH:mm:ss.SSS'));
        behData{index} = [timeString, ' -> ', event];  
        
        %stream data to the command window
        disp(behData{index});
        index = index + 1;
    end

    if recordVideo || streamVideo
    if vid.FramesAvailable > 1     
        theseFrames = getdata(vid, vid.FramesAvailable);
        totalFrames = cat(4,totalFrames,theseFrames);
    end
    end
    
    %check to see if user has clicked "End Session" button
    if ~ishandle(h5)
        disp('Session Ended');
        break;
    end

    %check if time elapsed is greater than the session 
    timeElapsed = datetime(now,'ConvertFrom','datenum', 'Format', 'HH:mm:ss.SSS') - behStartTime;
    if timeElapsed > sessionLength
    disp('Session Ended');
    close(fig2);

    %create a new video preview so user can still quickly observe the
    %session
     if recordVideo || streamVideo
    fig3 = figure('Name', behaviorBoxID); 
    hImage = image( zeros(vidRes(2), vidRes(1), nBands) ); 
    preview(vid, hImage); 

    h5 = uicontrol(fig3,...
    'Style','text',...
    'String', 'Session Ended',...
    'Position', [150 390 300 20], 'FontSize', 10);
    uicontrol('String', 'Close', 'Callback', 'close(gcf)');  
    break;
    end
    end
end

%% save and  plot data
behData = transpose(behData);
save behData behData baseFileName behStartTime;
delete(instrfindall);%close the serial port

%plot behavioral data 
figure;[summary, trials] = readAnalyzePlotBehData_adjustTTL(behData, baseFileName, sessionType, optoStim, 0.2);

%% stop and save video
if recordVideo
stop(vid)
numFrames = size(totalFrames, 4);
for ii = 1:numFrames
    writeVideo(diskLogger, totalFrames(:,:,:,ii));
end
close(diskLogger);
save vidStartTime vidStartTime
end
%stoppreview(vid)
%closepreview(vid)

%% ask user to copy data to project folder
if copyToProjectFolder
workingFolder = cd;
disp('Select the Project folder to copy your files to.')
projectFolder = uigetdir(sharedDriveStartPath);
folderContents = dir;
for i = 3:length(folderContents)
    copyfile(folderContents(i).name, projectFolder)
end
cd(workingFolder);
end
