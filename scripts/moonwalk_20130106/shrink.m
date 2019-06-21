function ret = shrink(matrix, kernelSize)
	r = floor(kernelSize(1) / 2);
	c = floor(kernelSize(2) / 2);

	if size(matrix,1) < r * 2 || size(matrix,2) < c * 2;
		error('cannot shrink matrix because it is too small');
	end
	
	ret = matrix(1+r:end-r,1+c:end-c);
end