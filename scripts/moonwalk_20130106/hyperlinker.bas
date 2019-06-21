Attribute VB_Name = "hyperlinker"
Public Sub Convert_To_Hyperlinks()
    Dim Cell As Range
    For Each Cell In Intersect(Selection, ActiveSheet.UsedRange)
        If Cell <> "" Then
            ActiveSheet.Hyperlinks.Add Cell, Cell.Value
        End If
    Next
End Sub

Public Sub Convert_From_Hyperlinks()
    Intersect(Selection, ActiveSheet.UsedRange).Hyperlinks.Delete
End Sub
