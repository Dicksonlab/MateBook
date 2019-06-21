function results = loadVideo(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo, path, results)
	if nargin < 5
		results.video = struct([]);
	end
	results.video(length(results.video) + 1).directoryName = path;
	
	arenaListing = dir(path);
	arenaListing = arenaListing([arenaListing.isdir] == 1);	% keep only directories
	arenaListing = arenaListing(~(strcmp({arenaListing.name}, '.') | strcmp({arenaListing.name}, '..')));	% remove . and ..
	arenaNumber = 0;
	for arena = arenaListing'
		arenaNumber = arenaNumber + 1;
		results.video(length(results.video)).arena(arenaNumber).directoryName = arena.name;

		trackPath = [path '/' arena.name '/track'];

		frameAttributeListing = dir([trackPath '/frame/']);
		for frameAttribute = frameAttributeListing'
			if isKey(frameAttributeInfo, frameAttribute.name)
				fileId = fopen([trackPath '/frame/' frameAttribute.name]);
				data = fread(fileId, inf, frameAttributeInfo(frameAttribute.name).typeConversion);
				fclose(fileId);
				data = reshape(data, frameAttributeInfo(frameAttribute.name).componentCount, length(data) / frameAttributeInfo(frameAttribute.name).componentCount)';
				results.video(length(results.video)).arena(arenaNumber).frameAttribute.(frameAttribute.name) = data;
			end
		end

		flyListing = dir([trackPath '/fly/']);
		for fly = flyListing'
			if strcmp(fly.name, '.') || strcmp(fly.name, '..') || ~isdir([trackPath '/fly/' fly.name '/'])
				continue;
			end
			flyNumber = str2double(fly.name) + 1;	% MATLAB indices start from 1
			if ~(mod(flyNumber, 1) == 0) || flyNumber < 1	% see if the directory name could be converted to a positive integer
				continue;
			end
			flyAttributeListing = dir([trackPath '/fly/' fly.name '/']);
			for flyAttribute = flyAttributeListing'
				if isKey(flyAttributeInfo, flyAttribute.name)
					fileId = fopen([trackPath '/fly/' fly.name '/' flyAttribute.name]);
					data = fread(fileId, inf, flyAttributeInfo(flyAttribute.name).typeConversion);
					fclose(fileId);
					data = reshape(data, flyAttributeInfo(flyAttribute.name).componentCount, length(data) / flyAttributeInfo(flyAttribute.name).componentCount)';
					results.video(length(results.video)).arena(arenaNumber).flyAttribute(flyNumber).(flyAttribute.name) = data;
				end
			end
		end

		activeListing = dir([trackPath '/pair/']);
		for active = activeListing'
			if strcmp(active.name, '.') || strcmp(active.name, '..') || ~isdir([trackPath '/pair/' active.name '/'])
				continue;
			end
			activeNumber = str2double(active.name) + 1;	% MATLAB indices start from 1
			if ~(mod(activeNumber, 1) == 0) || activeNumber < 1	% see if the directory name could be converted to a positive integer
				continue;
			end
			passiveListing = dir([trackPath '/pair/' active.name '/']);
			for passive = passiveListing'
				if strcmp(passive.name, '.') || strcmp(passive.name, '..') || ~isdir([trackPath '/pair/' active.name '/' passive.name '/'])
					continue;
				end
				passiveNumber = str2double(passive.name) + 1;	% MATLAB indices start from 1
				if ~(mod(passiveNumber, 1) == 0) || passiveNumber < 1	% see if the directory name could be converted to a positive integer
					continue;
				end
				pairAttributeListing = dir([trackPath '/pair/' active.name '/' passive.name '/']);
				for pairAttribute = pairAttributeListing'
					if isKey(pairAttributeInfo, pairAttribute.name)
						fileId = fopen([trackPath '/pair/' active.name '/' passive.name '/' pairAttribute.name]);
						data = fread(fileId, inf, pairAttributeInfo(pairAttribute.name).typeConversion);
						fclose(fileId);
						data = reshape(data, pairAttributeInfo(pairAttribute.name).componentCount, length(data) / pairAttributeInfo(pairAttribute.name).componentCount)';
						results.video(length(results.video)).arena(arenaNumber).pairAttribute(activeNumber,passiveNumber).(pairAttribute.name) = data;
					end
				end
			end
		end
	end
end