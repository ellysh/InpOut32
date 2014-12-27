Imports System.Runtime.InteropServices
Imports System.Threading

Public Class Form1

    <DllImport("InpOut32.dll", CharSet:=CharSet.Auto, EntryPoint:="Inp32")> _
    Shared Function Inp32(ByVal PortAddress As Short) As Short
    End Function

    <DllImport("InpOut32.dll", CharSet:=CharSet.Auto, EntryPoint:="Out32")> _
    Shared Sub Out32(ByVal PortAddress As Short, ByVal Data As Short)
    End Sub

    <DllImport("InpOut32.dll", CharSet:=CharSet.Auto, EntryPoint:="IsInpOutDriverOpen")> _
    Shared Function IsInpOutDriverOpen() As UInt32
    End Function

    <DllImport("InpOutx64.dll", CharSet:=CharSet.Auto, EntryPoint:="Inp32")> _
    Shared Function Inp32_x64(ByVal PortAddress As Short) As Short
    End Function

    <DllImport("InpOutx64.dll", CharSet:=CharSet.Auto, EntryPoint:="Out32")> _
    Shared Sub Out32_x64(ByVal PortAddress As Short, ByVal Data As Short)
    End Sub

    <DllImport("InpOutx64.dll", CharSet:=CharSet.Auto, EntryPoint:="IsInpOutDriverOpen")> _
    Shared Function IsInpOutDriverOpen_x64() As UInt32
    End Function


    Dim m_bX64 As Boolean = False

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        Try
            Dim iPort As Short
            iPort = Convert.ToInt16(TextBox1.Text)

            If (m_bX64) Then
                TextBox2.Text = Inp32_x64(iPort).ToString()
            Else
                TextBox2.Text = Inp32(iPort).ToString()
            End If

        Catch ex As Exception
            MessageBox.Show("An error occured:\n" + ex.Message)
        End Try
    End Sub

    Private Sub Button2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button2.Click
        Try
            Dim iPort As Short
            Dim iData As Short

            iPort = Convert.ToInt16(TextBox1.Text)
            iData = Convert.ToInt16(TextBox2.Text)

            If (m_bX64) Then
                Out32_x64(iPort, iData)
            Else
                Out32(iPort, iData)
            End If

        Catch ex As Exception
            MessageBox.Show("An error occured:\n" + ex.Message)
        End Try
    End Sub

    Private Sub Form1_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        Dim nResult As UInt32


        Try
            nResult = IsInpOutDriverOpen()
        Catch ex As Exception
            nResult = IsInpOutDriverOpen_x64()
            If (nResult <> 0) Then
                m_bX64 = True
            End If
        End Try

        If (nResult = 0) Then
            Label1.Text = "Unable to open InpOut driver"
        End If
    End Sub

    Private Sub Beep(ByVal freq As UInt32)
        If (m_bX64) Then
            Out32_x64(&H43, &HB6)
            Out32_x64(&H42, (freq And &HFF))
            Out32_x64(&H42, (freq >> 9))
            System.Threading.Thread.Sleep(10)
            Out32_x64(&H61, (Convert.ToByte(Inp32_x64(&H61)) Or &H3))
        Else
            Out32(&H43, &HB6)
            Out32(&H42, (freq And &HFF))
            Out32(&H42, (freq >> 9))
            System.Threading.Thread.Sleep(10)
            Out32(&H61, (Convert.ToByte(Inp32(&H61)) Or &H3))
        End If
    End Sub

    Private Sub StopBeep()
        If (m_bX64) Then
            Out32_x64(&H61, (Convert.ToByte(Inp32_x64(&H61)) And &HFC))
        Else
            Out32(&H61, (Convert.ToByte(Inp32(&H61)) And &HFC))
        End If
    End Sub

    Private Sub ThreadBeeper()
        Dim i As UInteger
        For i = 440000 To 500000 Step 1000
            Dim freq As UInteger = 1193180000 / i '440Hz
            Beep(freq)
        Next i
        StopBeep()
    End Sub


    Private Sub button7_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles button7.Click
        Dim t As New System.Threading.Thread(New System.Threading.ThreadStart(AddressOf ThreadBeeper))
        t.Start()
    End Sub
End Class
