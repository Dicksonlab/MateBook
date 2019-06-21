function ret = medfilt2_clamp(matrix, neighborhood)
	ret = grow(matrix, neighborhood);
	ret = medfilt2(ret, neighborhood);
	ret = shrink(ret, neighborhood);
end