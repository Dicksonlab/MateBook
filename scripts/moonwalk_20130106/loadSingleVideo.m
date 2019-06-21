function arena = loadSingleVideo(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo, path)
	arena = struct([]);
	trackListing = dir([path '/*_track']);
	for track = trackListing'
		% get the arena number
		arenaNumber = str2double(track.name(1:(length(track.name)-length('_track')))) + 1;	% MATLAB indices start from 1
		if ~(mod(arenaNumber, 1) == 0) || arenaNumber < 1	% see if the string could be converted to a positive integer
			continue;
		end

		frameAttributeListing = dir([path '/' track.name '/frame/']);
		for frameAttribute = frameAttributeListing'
			if isKey(frameAttributeInfo, frameAttribute.name)
				fileId = fopen([path '/' track.name '/frame/' frameAttribute.name]);
				data = fread(fileId, inf, frameAttributeInfo(frameAttribute.name).typeConversion);
				fclose(fileId);
				data = reshape(data, frameAttributeInfo(frameAttribute.name).componentCount, length(data) / frameAttributeInfo(frameAttribute.name).componentCount)';
				arena(arenaNumber).frameAttribute.(frameAttribute.name) = data;
			end
		end

		flyListing = dir([path '/' track.name '/fly/']);
		for fly = flyListing'
			if strcmp(fly.name, '.') || strcmp(fly.name, '..') || ~isdir([path '/' track.name '/fly/' fly.name '/'])
				continue;
			end
			flyNumber = str2double(fly.name) + 1;	% MATLAB indices start from 1
			if ~(mod(flyNumber, 1) == 0) || flyNumber < 1	% see if the directory name could be converted to a positive integer
				continue;
			end
			flyAttributeListing = dir([path '/' track.name '/fly/' fly.name '/']);
			for flyAttribute = flyAttributeListing'
				if isKey(flyAttributeInfo, flyAttribute.name)
					fileId = fopen([path '/' track.name '/fly/' fly.name '/' flyAttribute.name]);
					data = fread(fileId, inf, flyAttributeInfo(flyAttribute.name).typeConversion);
					fclose(fileId);
					data = reshape(data, flyAttributeInfo(flyAttribute.name).componentCount, length(data) / flyAttributeInfo(flyAttribute.name).componentCount)';
					arena(arenaNumber).flyAttribute(flyNumber).(flyAttribute.name) = data;
				end
			end
		end

		activeListing = dir([path '/' track.name '/pair/']);
		for active = activeListing'
			if strcmp(active.name, '.') || strcmp(active.name, '..') || ~isdir([path '/' track.name '/pair/' active.name '/'])
				continue;
			end
			activeNumber = str2double(active.name) + 1;	% MATLAB indices start from 1
			if ~(mod(activeNumber, 1) == 0) || activeNumber < 1	% see if the directory name could be converted to a positive integer
				continue;
			end
			passiveListing = dir([path '/' track.name '/pair/' active.name '/']);
			for passive = passiveListing'
				if strcmp(passive.name, '.') || strcmp(passive.name, '..') || ~isdir([path '/' track.name '/pair/' active.name '/' passive.name '/'])
					continue;
				end
				passiveNumber = str2double(passive.name) + 1;	% MATLAB indices start from 1
				if ~(mod(passiveNumber, 1) == 0) || passiveNumber < 1	% see if the directory name could be converted to a positive integer
					continue;
				end
				pairAttributeListing = dir([path '/' track.name '/pair/' active.name '/' passive.name '/']);
				for pairAttribute = pairAttributeListing'
					if isKey(pairAttributeInfo, pairAttribute.name)
						fileId = fopen([path '/' track.name '/pair/' active.name '/' passive.name '/' pairAttribute.name]);
						data = fread(fileId, inf, pairAttributeInfo(pairAttribute.name).typeConversion);
						fclose(fileId);
						data = reshape(data, pairAttributeInfo(pairAttribute.name).componentCount, length(data) / pairAttributeInfo(pairAttribute.name).componentCount)';
						arena(arenaNumber).pairAttribute(activeNumber,passiveNumber).(pairAttribute.name) = data;
					end
				end
			end
		end
	end
end