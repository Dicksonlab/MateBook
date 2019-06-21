function xlsclose(xlshandle)
% XLSCLOSE Saves and closes Excel workbook.
%==============================================================================
	xlshandle.Excel.DisplayAlerts = 0;
	xlshandle.ExcelWorkbook.Save;
	xlshandle.ExcelWorkbook.Close(false);
	xlshandle.Excel.Quit;
end
