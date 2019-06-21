function motionPlots(inDirectory, outDirectory)
	% input file names
	inFile = 'motion.csv';
	headingAnnotationFile = 'heading.txt';
	arenaDescriptionFile = 'arenas.csv';

	% output file names
	motionPlotFile = 'motion.png';
	statisticsFile = 'statistics.xlsx';

	% video information
	fps = 25;
	videoWidth = 1920;	% pixels
	arenaWidth = 75;	% mm
	
	% formatting
	motionPlotSize = [1280 900];
	fileFontSize = 14;	% the size video file names (and paths) are printed in
	labelFontSize = 10;
	lineThickness = 5;
	figureBackgroundColor = [0.98 0.98 0.98];
	headingColormap = [255 52 150; 0 0 0; 52 109 255] / 255;
	forwardColor = [0.1 0.8 0.5];
	backwardColor = [0.7 0 0.9];
	stateTransparency = 0.3;	% transparency of state polygons, 0.2 looks nice
	drawTransitions = true;
	
	% what input to process
	processResultsWithHeadingAnnotationFileOnly = false;
	skipIfResultsExist = false;
	skipIfResultsNewerThan = '09-Mar-2012 22:45:00';	% set to '' to disable
	normalVideoLength = 600;
	skipShorterVideos = true;
	truncateLongerVideos = true;

	% inFile format description
	statsPerFly = 6;	% that is, columns per fly in the inFile
	statsOffset = 1;	% the time column offsets the fly attributes in inFile by 1 column
	timeColumn = 1;
	
	% postprocessing options
	stallThreshold = 1.46484375;	% if the fly moves slower than that (in mm/s) it will be counted as a stall
	medfiltSize = 13;	% the size of the median filter used to smoothen motion states
	movedFastMedfiltSize = 13; % the size of the median filter used to smoothen the movedFast parameter
	removeIslands = 49;	% structuring element size for opening
	bridgeGaps = 25;	% structuring element size for closing
	transitionDef = {...
		{{'stall' 'o' [0 0 0]}				{'forward turn' 's' [0 1 0]}		{'forward flip' 'o' [0 1 0]}	{'forward reverse' 'o' [1 1 0]}},...
		{{'forward turn' 's' [0 1 0]}		{'stall' 'o' [0 0 0]}				{'forward reverse' 'o' [1 1 0]}	{'forward flip' 'o' [0 1 0]}},...
		{{'backward flip' 'o' [1 0 0]}		{'backward reverse' 'o' [0 1 1]}	{'stall' 'o' [0 0 0]}			{'backward turn' 's' [1 0 0]}},...
		{{'backward reverse' 'o' [0 1 1]}	{'backward flip' 'o' [1 0 0]}		{'backward turn' 's' [1 0 0]}	{'stall' 'o' [0 0 0]}},...
	};
	stateCount = length(transitionDef);
	forwardLeftState = 1;
	forwardRightState = 2;
	backwardLeftState = 3;
	backwardRightState = 4;
	periphery = 1 / 4;	% the part of the image that is defined to be the periphery both on the left and on the right
	forced = 3;	% mm the fly body centroid may be away from the border for transitions to be counted as forced

	if nargin < 1
		motionPlots('.', '.');
		return;
	end
	
	listing = dir(inDirectory);
	for entry = listing'
		if strcmp(entry.name, '.') || strcmp(entry.name, '..')
			continue;
		end
		
		if isdir([inDirectory '/' entry.name])
			[success, message, messageId] = mkdir(outDirectory, entry.name);
			if success
				motionPlots([inDirectory '/' entry.name], [outDirectory '/' entry.name]);
			else
				warning(messageId, message);
				disp(['...not decending into directory ' inDirectory '/' entry.name]);
			end
		end
	end
	
	if skipIfResultsExist && exist([outDirectory '/' statisticsFile], 'file') && exist([outDirectory '/' motionPlotFile], 'file')
		disp(['skipping ' inFile ' in ' inDirectory ' because the results exist already']);
		return;
	end

	listing = dir([inDirectory '/' inFile]);
	if length(listing) > 1
		disp(['more than one ' inFile ' found in directory ' inDirectory ' ...sanity check failed, returning']);
		return;
	elseif length(listing) == 1
		if ~isempty(skipIfResultsNewerThan)
			statisticsListing = dir([outDirectory '/' statisticsFile]);
			if length(statisticsListing) > 1
				disp(['more than one ' statisticsFile ' found in directory ' outDirectory ' ...sanity check failed, returning']);
				return;
			elseif length(statisticsListing) == 1 && datenum(statisticsListing(1).date) > datenum(skipIfResultsNewerThan)
				disp(['skipping ' inFile ' in ' inDirectory ' because the existing results are from ' statisticsListing(1).date]);
				return;
			end
		end
		if processResultsWithHeadingAnnotationFileOnly && ~exist([inDirectory '/' headingAnnotationFile], 'file')
			return;
		end
		disp(['processing ' inFile ' in ' inDirectory]);
		if exist([outDirectory '/' statisticsFile], 'file')	% since xlswrite does not create a new file, we have to delete the old one
			delete([outDirectory '/' statisticsFile]);
		end
		motion = xlsread([inDirectory '/' inFile]);
		frameCount = size(motion, 1);

		if skipShorterVideos && frameCount < normalVideoLength * fps
			disp(['skipping it because it is shorter than ' num2str(normalVideoLength) ' seconds']);
			return;
		end
		
		if truncateLongerVideos && frameCount > normalVideoLength * fps
			motion = motion(1:normalVideoLength*fps,:);
			frameCount = normalVideoLength * fps;
		end
		
		% downsampling - do not use: statistical analysis doesn't take this into account the way it is implemented currently
		%motion = motion(1:25:end,:);

		flyCount = (size(motion, 2) - statsOffset) / statsPerFly;
		time = motion(:,timeColumn);
		
		if motionPlotFile
			figure('Visible', 'off');
		else
			figure;
		end
		
		headingAnnotation = [];
		try
			headingAnnotation = parseHeading([inDirectory '/' headingAnnotationFile], frameCount, fps);
		catch e
		end

		arenaWidthPixels = repmat(videoWidth, [flyCount 1]);
		try
			arenaDescription = xlsread([inDirectory '/' arenaDescriptionFile]);
			if ~all(size(arenaDescription) >= [flyCount 4])	% note that we're assuming that the flies are put in the arenas from the top down
				error(['parsing ' arenaDescriptionFile]);
			end
			arenaWidthPixels = arenaDescription(:,3);
		catch e
			disp(['could not read arena description file ' arenaDescriptionFile ' ...assuming arenas are ' num2str(videoWidth) ' pixels wide']);
		end

		for fly = 1 : flyCount;
			position = motion(:,statsOffset+(fly-1)*statsPerFly+2) * arenaWidth / arenaWidthPixels(fly);	% in mm
			velocity = motion(:,statsOffset+(fly-1)*statsPerFly+4) * arenaWidth / arenaWidthPixels(fly) * fps;	% in mm/s
			heading = motion(:,statsOffset+(fly-1)*statsPerFly+5);
			bodySize = motion(:,statsOffset+(fly-1)*statsPerFly+6) * (arenaWidth / arenaWidthPixels(fly))^2;	% in mm^2
			
			movedDirection = sign(velocity);
			
			movedForward = (heading == movedDirection);
			movedBackward = (heading ~= movedDirection);
			movedLeft = (movedDirection == -1);
			movedRight = (movedDirection == 1);
			movedFast = abs(velocity) > stallThreshold;
			
			movedForward = medfilt2_clamp(movedForward, [medfiltSize 1]);
			movedBackward = medfilt2_clamp(movedBackward, [medfiltSize 1]);
			movedLeft = medfilt2_clamp(movedLeft, [medfiltSize 1]);
			movedRight = medfilt2_clamp(movedRight, [medfiltSize 1]);
			movedFast = medfilt2_clamp(movedFast, [movedFastMedfiltSize 1]);
			
			forwardLeft = movedForward & movedLeft & movedFast;
			forwardRight = movedForward & movedRight & movedFast;
			backwardLeft = movedBackward & movedLeft & movedFast;
			backwardRight = movedBackward & movedRight & movedFast;

			if removeIslands
				openStrel = strel('line', removeIslands, 90);
				forwardLeft = imopen(forwardLeft, openStrel);
				forwardRight = imopen(forwardRight, openStrel);
				backwardLeft = imopen(backwardLeft, openStrel);
				backwardRight = imopen(backwardRight, openStrel);
			end
			
			if bridgeGaps
				closeStrel = strel('line', bridgeGaps, 90);
				forwardLeft = imclose(forwardLeft, closeStrel);
				forwardRight = imclose(forwardRight, closeStrel);
				backwardLeft = imclose(backwardLeft, closeStrel);
				backwardRight = imclose(backwardRight, closeStrel);
			end
			
			flBegin = forwardLeft & ~[false; forwardLeft(1:end-1)];
			flEnd = forwardLeft & ~[forwardLeft(2:end); false];
			frBegin = forwardRight & ~[false; forwardRight(1:end-1)];
			frEnd = forwardRight & ~[forwardRight(2:end); false];
			blBegin = backwardLeft & ~[false; backwardLeft(1:end-1)];
			blEnd = backwardLeft & ~[backwardLeft(2:end); false];
			brBegin = backwardRight & ~[false; backwardRight(1:end-1)];
			brEnd = backwardRight & ~[backwardRight(2:end); false];
			
			flBeginTime = time(flBegin);
			flEndTime = time(flEnd);
			frBeginTime = time(frBegin);
			frEndTime = time(frEnd);
			blBeginTime = time(blBegin);
			blEndTime = time(blEnd);
			brBeginTime = time(brBegin);
			brEndTime = time(brEnd);
			
			subplotHorizontalMargin = 0.05;
			subplotVerticalMargin = 0.08;
			subplotSpacing = 0.01;
			subplotWidth = (1 - 2 * subplotHorizontalMargin - (flyCount - 1) * subplotSpacing) / flyCount;
			subplotHeight = 1 - 2 * subplotVerticalMargin;
			subplotBottom = subplotVerticalMargin;
			subplotLeft = subplotHorizontalMargin + (fly - 1) * (subplotSpacing + subplotWidth);
			axes('YDir', 'reverse', 'Position', [subplotLeft, subplotBottom, subplotWidth, subplotHeight]);
			hold on;

%% draw transition markers
			state = forwardLeft + 2 * forwardRight + 3 * backwardLeft + 4 * backwardRight;
			state = [state (1:length(state))'];	% taking the indexes with us
			stateBegin = state([1; diff(state(:,1))]~=0,:);	% keep only the frames where states begin
			stateEnd = state([diff(state(:,1)); 1]~=0,:);	% keep only the frames where states end
			stateBegin = stateBegin(stateBegin(:,1)~=0,:); % removing the passive state
			stateEnd = stateEnd(stateEnd(:,1)~=0,:); % removing the passive state

			transition = [1 1 1 frameCount];	% for flies that do not enter any state we initialize this with one long stall
			if ~isempty(stateBegin)
				transition = [stateEnd(1:end-1,:) stateBegin(2:end,:)];	% end of prior state to beginning of next state
				if stateBegin(1,2) ~= 1	% if it does not begin with a state
					transition = [...
						[stateBegin(1,1) 1] stateBegin(1,:);...	% make the first transition (where no prior state is known) a stall
						transition;...
					];
				end
				if stateEnd(end,2) ~= frameCount	% if it does not end in a state
					transition = [...
						transition;...
						stateEnd(end,:) [stateEnd(end,1) frameCount];...	% make the last transition (where no next state is known) a stall
					];
				end
			end
			
			if drawTransitions
				for priorState = 1:stateCount
					for nextState = 1:stateCount
						thisTransitionMask = transition(:,1) == priorState & transition(:,3) == nextState;
						thisTransition = round((transition(thisTransitionMask,2) + transition(thisTransitionMask,4)) / 2);	% mark the midpoint of a transition
%						thisTransition = transition(thisTransitionMask,4);	% mark the end of a transition
						plot(position(thisTransition), time(thisTransition),...
							transitionDef{nextState}{priorState}{2},...	% marker type
							'LineWidth', 0.1,...	% also affects the marker edge
							'MarkerEdgeColor', 'k',...
							'MarkerFaceColor', transitionDef{nextState}{priorState}{3},...
							'MarkerSize', 5);
					end
				end
			end
			
%% draw states
			width = arenaWidth;
			xdata = repmat([width; 0; 0; width], [1 length(flBeginTime)]);
			ydata = [flBeginTime'; flBeginTime'; flEndTime'; flEndTime'];
			zdata = -ones(4, length(flBeginTime));
			p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(frBeginTime)]);
			ydata = [frBeginTime'; frBeginTime'; frEndTime'; frEndTime'];
			zdata = -ones(4, length(frBeginTime));
			p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(blBeginTime)]);
			ydata = [blBeginTime'; blBeginTime'; blEndTime'; blEndTime'];
			zdata = -ones(4, length(blBeginTime));
			p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(brBeginTime)]);
			ydata = [brBeginTime'; brBeginTime'; brEndTime'; brEndTime'];
			zdata = -ones(4, length(brBeginTime));
			p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?

%% draw position and heading
			scatter(position, time, lineThickness, heading, 'filled');
			caxis([-1 1]);
			grid('on');
			box('on');
			axis([0 width 0 time(end)])
			set(gca, 'FontSize', labelFontSize);
			title(['Fly ' num2str(fly)]);
			if fly == 1
				xlabel('x coordinate (mm)');
				ylabel('time (s)');
			end
			set(gca, 'XTick', [0 round(width*periphery) round(width*(1-periphery)) width]);
			set(gca, 'YTick', 0:60:time(end));
			if fly ~= 1
				set(gca, 'YTickLabel', []);
			end
			
			if fly == 1
				[filePath, fileName, fileExt] = fileparts(inDirectory);
				text(0, -time(end) / 20, [fileName fileExt ' (' filePath ')'], 'Interpreter', 'none', 'FontSize', fileFontSize, 'FontWeight', 'bold');
			end
			
%% statistics
			% from tracking data
			moved = abs(velocity) / fps;
			absMoved = abs(moved);
			totalForwardMovement = nansum(absMoved(movedForward));
			totalBackwardMovement = nansum(absMoved(movedBackward));
			totalMovement = totalForwardMovement + totalBackwardMovement;
			backwardOverForward = totalBackwardMovement / totalForwardMovement;
			backwardOverTotal = totalBackwardMovement / totalMovement;
			forwardOverTotal = totalForwardMovement / totalMovement;

			inPeriphery = position < width * periphery | position > width * (1-periphery);
			absMovedPeriphery = absMoved(inPeriphery);
			movedForwardPeriphery = movedForward(inPeriphery);
			movedBackwardPeriphery = movedBackward(inPeriphery);
			totalForwardMovementPeriphery = nansum(absMovedPeriphery(movedForwardPeriphery));
			totalBackwardMovementPeriphery = nansum(absMovedPeriphery(movedBackwardPeriphery));
			totalMovementPeriphery = totalForwardMovementPeriphery + totalBackwardMovementPeriphery;
			backwardOverForwardPeriphery = totalBackwardMovementPeriphery / totalForwardMovementPeriphery;
			backwardOverTotalPeriphery = totalBackwardMovementPeriphery / totalMovementPeriphery;
			forwardOverTotalPeriphery = totalForwardMovementPeriphery / totalMovementPeriphery;

			inCenter = ~inPeriphery;
			absMovedCenter = absMoved(inCenter);
			movedForwardCenter = movedForward(inCenter);
			movedBackwardCenter = movedBackward(inCenter);
			totalForwardMovementCenter = nansum(absMovedCenter(movedForwardCenter));
			totalBackwardMovementCenter = nansum(absMovedCenter(movedBackwardCenter));
			totalMovementCenter = totalForwardMovementCenter + totalBackwardMovementCenter;
			backwardOverForwardCenter = totalBackwardMovementCenter / totalForwardMovementCenter;
			backwardOverTotalCenter = totalBackwardMovementCenter / totalMovementCenter;
			forwardOverTotalCenter = totalForwardMovementCenter / totalMovementCenter;
			
			timeInPeriphery = nnz(inPeriphery) / fps;
			
			numberOfHeadingChanges = nnz(diff(heading));
			
			meanBodySize = nanmean(bodySize);
			
			inLeftEndZone = position < forced;
			inRightEndZone = position > (width - forced);

			% from ethogram
			timeInFLFR = (nnz(forwardLeft) + nnz(forwardRight)) / fps;
			timeInBLBR = (nnz(backwardLeft) + nnz(backwardRight)) / fps;
			timeInFLFRBLBR = timeInFLFR + timeInBLBR;
			timeInBLBROverFLFR = timeInBLBR / timeInFLFR;
			timeInBLBROverFLFRBLBR = timeInBLBR / timeInFLFRBLBR;
			timeInFLFROverFLFRBLBR = timeInFLFR / timeInFLFRBLBR;
			
			numForwardReversals = 0;
			numBackwardReversals = 0;
			numForwardTurns = 0;
			numBackwardTurns = 0;
			numForwardFlips = 0;
			numBackwardFlips = 0;
			numStalls = 0;
			numForwardReversalsPeriphery = 0;
			numBackwardReversalsPeriphery = 0;
			numForwardTurnsPeriphery = 0;
			numBackwardTurnsPeriphery = 0;
			numForwardFlipsPeriphery = 0;
			numBackwardFlipsPeriphery = 0;
			numStallsPeriphery = 0;
			numForwardReversalsCenter = 0;
			numBackwardReversalsCenter = 0;
			numForwardTurnsCenter = 0;
			numBackwardTurnsCenter = 0;
			numForwardFlipsCenter = 0;
			numBackwardFlipsCenter = 0;
			numStallsCenter = 0;
			numForwardReversalsForced = 0;
			numBackwardReversalsForced = 0;
			numForwardTurnsForced = 0;
			numBackwardTurnsForced = 0;
			numForwardFlipsForced = 0;
			numBackwardFlipsForced = 0;
			numStallsForced = 0;
			reversalDuration = 0;
			turnDuration = 0;
			flipDuration = 0;
			stallDuration = 0;
			stallDurationForced = 0;
			for priorState = 1:stateCount
				for nextState = 1:stateCount
					thisTransitionMask = transition(:,1) == priorState & transition(:,3) == nextState;
					thisTransition = transition(thisTransitionMask,:);
					thisTransitionName = transitionDef{nextState}{priorState}{1};
					midIndexes = round((thisTransition(:,2) + thisTransition(:,4)) / 2);
					if (strcmp(thisTransitionName, 'stall'))
						numStalls = numStalls + size(thisTransition, 1);
						numStallsPeriphery = numStallsPeriphery + nnz(inPeriphery(midIndexes));
						numStallsCenter = numStallsCenter + nnz(inCenter(midIndexes));
						forcedStallLeft = inLeftEndZone(midIndexes) & heading(midIndexes) == -1;
						forcedStallRight = inRightEndZone(midIndexes) & heading(midIndexes) == 1;
						numStallsForced = numStallsForced + nnz(forcedStallLeft) + nnz(forcedStallRight);
						stallDurationForced = stallDurationForced + sum(thisTransition(forcedStallLeft,4) - thisTransition(forcedStallLeft,2));
						stallDurationForced = stallDurationForced + sum(thisTransition(forcedStallRight,4) - thisTransition(forcedStallRight,2));
						stallDuration = stallDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward reverse')
						numForwardReversals = numForwardReversals + size(thisTransition, 1);
						numForwardReversalsPeriphery = numForwardReversalsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardReversalsCenter = numForwardReversalsCenter + nnz(inCenter(midIndexes));
						numForwardReversalsForced = numForwardReversalsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						reversalDuration = reversalDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward reverse')
						numBackwardReversals = numBackwardReversals + size(thisTransition, 1);
						numBackwardReversalsPeriphery = numBackwardReversalsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardReversalsCenter = numBackwardReversalsCenter + nnz(inCenter(midIndexes));
						numBackwardReversalsForced = numBackwardReversalsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						reversalDuration = reversalDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward turn')
						numForwardTurns = numForwardTurns + size(thisTransition, 1);
						numForwardTurnsPeriphery = numForwardTurnsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardTurnsCenter = numForwardTurnsCenter + nnz(inCenter(midIndexes));
						numForwardTurnsForced = numForwardTurnsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						turnDuration = turnDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward turn')
						numBackwardTurns = numBackwardTurns + size(thisTransition, 1);
						numBackwardTurnsPeriphery = numBackwardTurnsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardTurnsCenter = numBackwardTurnsCenter + nnz(inCenter(midIndexes));
						numBackwardTurnsForced = numBackwardTurnsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						turnDuration = turnDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward flip')
						numForwardFlips = numForwardFlips + size(thisTransition, 1);
						numForwardFlipsPeriphery = numForwardFlipsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardFlipsCenter = numForwardFlipsCenter + nnz(inCenter(midIndexes));
						numForwardFlipsForced = numForwardFlipsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						flipDuration = flipDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward flip')
						numBackwardFlips = numBackwardFlips + size(thisTransition, 1);
						numBackwardFlipsPeriphery = numBackwardFlipsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardFlipsCenter = numBackwardFlipsCenter + nnz(inCenter(midIndexes));
						numBackwardFlipsForced = numBackwardFlipsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						flipDuration = flipDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					else
						disp('transition not counted:');
						thisTransition
					end
				end
			end
			numReversals = numForwardReversals + numBackwardReversals;
			numReversalsPeriphery = numForwardReversalsPeriphery + numBackwardReversalsPeriphery;
			numReversalsCenter = numForwardReversalsCenter + numBackwardReversalsCenter;
			numReversalsForced = numForwardReversalsForced + numBackwardReversalsForced;
			numTurns = numForwardTurns + numBackwardTurns;
			numTurnsPeriphery = numForwardTurnsPeriphery + numBackwardTurnsPeriphery;
			numTurnsCenter = numForwardTurnsCenter + numBackwardTurnsCenter;
			numTurnsForced = numForwardTurnsForced + numBackwardTurnsForced;
			numFlips = numForwardFlips + numBackwardFlips;
			numFlipsPeriphery = numForwardFlipsPeriphery + numBackwardFlipsPeriphery;
			numFlipsCenter = numForwardFlipsCenter + numBackwardFlipsCenter;
			numFlipsForced = numForwardFlipsForced + numBackwardFlipsForced;
			numTransitions = size(transition, 1);
			numTransitionsForced = numStallsForced + numReversalsForced + numTurnsForced + numFlipsForced;
			relReversals = numReversals / numTransitions;
			relTurns = numTurns / numTransitions;
			relFlips = numFlips / numTransitions;
			relStalls = numStalls / numTransitions;
			numReversalsOverNumTurnsNumReversals = numReversals / (numTurns + numReversals);
			numReversalsOverNumTurnsNumReversalsForced = numReversalsForced / (numTurnsForced + numReversalsForced);
			numReversalsOverNumTurnsNumReversalsNumFlipsForced = numReversalsForced / (numTurnsForced + numReversalsForced + numFlipsForced);
			numTurnsNumFlipsOverNumTransitionsForced = (numTurnsForced + numFlipsForced) / numTransitionsForced;
			numStallsForcedOverNumStalls = numStallsForced / numStalls;
			reversalDuration = reversalDuration / fps;
			turnDuration = turnDuration / fps;
			flipDuration = flipDuration / fps;
			stallDuration = stallDuration / fps;
			stallDurationForced = stallDurationForced / fps;
			meanReversalDuration = reversalDuration / numReversals;
			meanTurnDuration = turnDuration / numTurns;
			meanFlipDuration = flipDuration / numFlips;
			meanStallDuration = stallDuration / numStalls;
			meanStallDurationForced = stallDurationForced / numStallsForced;
			numForwardBouts = size(flBeginTime, 1) + size(frBeginTime, 1);
			numBackwardBouts = size(blBeginTime, 1) + size(brBeginTime, 1);
			meanForwardBoutDuration = timeInFLFR / numForwardBouts;
			meanBackwardBoutDuration = timeInBLBR / numBackwardBouts;
			
			headingCorrectness = NaN;
			if ~isempty(headingAnnotation)
				headingCorrectness = nnz(headingAnnotation(:,fly) == heading) / frameCount;
			end
			
			warning off MATLAB:xlswrite:AddSheet;
			xlswrite([outDirectory '/' statisticsFile], {['Fly ' num2str(fly)]}, 'statistics', [getExcelCol(fly+1) '1']);
			xlswrite([outDirectory '/' statisticsFile], [...
				totalForwardMovement;
				totalBackwardMovement;
				totalMovement;
				backwardOverForward;
				backwardOverTotal;
				forwardOverTotal;
				totalForwardMovementPeriphery;
				totalBackwardMovementPeriphery;
				totalMovementPeriphery;
				backwardOverForwardPeriphery;
				backwardOverTotalPeriphery;
				forwardOverTotalPeriphery;
				totalForwardMovementCenter;
				totalBackwardMovementCenter;
				totalMovementCenter;
				backwardOverForwardCenter;
				backwardOverTotalCenter;
				forwardOverTotalCenter;
				timeInPeriphery;
				numberOfHeadingChanges;
				meanBodySize;
				timeInFLFR;
				timeInBLBR;
				timeInFLFRBLBR;
				timeInBLBROverFLFR;
				timeInBLBROverFLFRBLBR;
				timeInFLFROverFLFRBLBR;
				numReversals;
				numForwardReversals;
				numBackwardReversals;
				numReversalsForced;
				numForwardReversalsForced;
				numBackwardReversalsForced;
				numReversalsPeriphery;
				numForwardReversalsPeriphery;
				numBackwardReversalsPeriphery;
				numReversalsCenter;
				numForwardReversalsCenter;
				numBackwardReversalsCenter;
				numTurns;
				numForwardTurns;
				numBackwardTurns;
				numTurnsForced;
				numForwardTurnsForced;
				numBackwardTurnsForced;
				numTurnsPeriphery;
				numForwardTurnsPeriphery;
				numBackwardTurnsPeriphery;
				numTurnsCenter;
				numForwardTurnsCenter;
				numBackwardTurnsCenter;
				numFlips;
				numForwardFlips;
				numBackwardFlips;
				numFlipsForced;
				numForwardFlipsForced;
				numBackwardFlipsForced;
				numFlipsPeriphery;
				numForwardFlipsPeriphery;
				numBackwardFlipsPeriphery;
				numFlipsCenter;
				numForwardFlipsCenter;
				numBackwardFlipsCenter;
				numStalls;
				numStallsForced;
				numStallsPeriphery;
				numStallsCenter;
				relReversals;
				relTurns;
				relFlips;
				relStalls;
				numReversalsOverNumTurnsNumReversals;
				numReversalsOverNumTurnsNumReversalsForced;
				numReversalsOverNumTurnsNumReversalsNumFlipsForced;
				numTurnsNumFlipsOverNumTransitionsForced;
				numStallsForcedOverNumStalls;
				reversalDuration;
				turnDuration;
				flipDuration;
				stallDuration;
				stallDurationForced;
				meanReversalDuration;
				meanTurnDuration;
				meanFlipDuration;
				meanStallDuration;
				meanStallDurationForced;
				numForwardBouts;
				numBackwardBouts;
				meanForwardBoutDuration;
				meanBackwardBoutDuration;
				headingCorrectness;
			], 'statistics', [getExcelCol(fly+1) '2']);
		end
		xlswrite([outDirectory '/' statisticsFile], {...
			'total forward movement';
			'total backward movement';
			'total movement';
			'backward/forward';
			'backward/total';
			'forward/total';
			'total forward movement in periphery';
			'total backward movement in periphery';
			'total movement in periphery';
			'backward/forward in periphery';
			'backward/total in periphery';
			'forward/total in periphery';
			'total forward movement in center';
			'total backward movement in center';
			'total movement in center';
			'backward/forward in center';
			'backward/total in center';
			'forward/total in center';
			'time in periphery';
			'number of heading changes';
			'mean body size';
			'time in FL|FR';
			'time in BL|BR';
			'time in FL|FR|BL|BR';
			'(time in BL|BR) / (time in FL|FR)';
			'(time in BL|BR) / (time in FL|FR|BL|BR)';
			'(time in FL|FR) / (time in FL|FR|BL|BR)';
			'#reversals';
			'#forward reversals';
			'#backward reversals';
			'#reversals forced';
			'#forward reversals forced';
			'#backward reversals forced';
			'#reversals in periphery';
			'#forward reversals in periphery';
			'#backward reversals in periphery';
			'#reversals in center';
			'#forward reversals in center';
			'#backward reversals in center';
			'#turns';
			'#forward turns';
			'#backward turns';
			'#turns forced';
			'#forward turns forced';
			'#backward turns forced';
			'#turns in periphery';
			'#forward turns in periphery';
			'#backward turns in periphery';
			'#turns in center';
			'#forward turns in center';
			'#backward turns in center';
			'#flips';
			'#forward flips';
			'#backward flips';
			'#flips forced';
			'#forward flips forced';
			'#backward flips forced';
			'#flips in periphery';
			'#forward flips in periphery';
			'#backward flips in periphery';
			'#flips in center';
			'#forward flips in center';
			'#backward flips in center';
			'#stalls';
			'#stalls forced';
			'#stalls in periphery';
			'#stalls in center';
			'#reversals / #transitions';
			'#turns / #transitions';
			'#flips / #transitions';
			'#stalls / #transitions';
			'#reversals / (#turns + #reversals)';
			'#reversals forced / (#turns forced + #reversals forced)';
			'#reversals forced / (#turns forced + #reversals forced + #flips forced)';
			'(#turns forced + #flips forced) / #transitions forced';
			'#stalls forced / #stalls';
			'total reversal duration';
			'total turn duration';
			'total flip duration';
			'total stall duration';
			'total stall duration forced';
			'mean reversal duration';
			'mean turn duration';
			'mean flip duration';
			'mean stall duration';
			'mean stall duration forced';
			'#forward bouts';
			'#backward bouts';
			'mean forward bout duration';
			'mean backward bout duration';
			'heading correctness';
		}, 'statistics', 'A2');
		
%		set(gcf, 'Position', [0, 0, 400, 300]);	% seems to have no effect when using "saveas"
		colormap(headingColormap);
		%myaa;
		if motionPlotFile
			set(gcf,...
				'Color', figureBackgroundColor,...
				'Position', [0 0 motionPlotSize],...
				'PaperPositionMode', 'auto',...
				'InvertHardcopy', 'off'...
			);
			print('-dpng', '-r0', [outDirectory '/' motionPlotFile]);
%			saveas(gcf, [outDirectory '/' motionPlotFile]);
		end
	end
end