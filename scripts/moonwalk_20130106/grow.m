function ret = grow(matrix, kernelSize)
	if isempty(matrix)
		error('cannot grow empty matrix since it has no borders');
	end
	
	r = floor(kernelSize(1) / 2);
	c = floor(kernelSize(2) / 2);
	
	ret = [...
		repmat(matrix(1,1), [r c])	repmat(matrix(1,:), [r 1])	repmat(matrix(1,end), [r c]);...
		repmat(matrix(:,1), [1 c])	matrix						repmat(matrix(:,end), [1 c]);...
		repmat(matrix(end,1), [r c])	repmat(matrix(end,:), [r 1])	repmat(matrix(end,end), [r c]);...
	];
	
end