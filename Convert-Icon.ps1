# Convert-Icon.ps1
# Converts checkpoint_icon.png -> checkpoint.ico using .NET
# Run once before building: .\Convert-Icon.ps1

param(
    [string]$PngPath = "$PSScriptRoot\checkpoint_icon.png",
    [string]$OutPath = "$PSScriptRoot\resources\checkpoint.ico"
)

Add-Type -AssemblyName System.Drawing

function ConvertTo-Icon {
    param($png, $outPath, [int[]]$sizes = @(256, 64, 48, 32, 16))

    $stream = [System.IO.File]::OpenWrite($outPath)
    $writer = [System.IO.BinaryWriter]::new($stream)

    $images = @()
    foreach ($size in $sizes) {
        $bmp = [System.Drawing.Bitmap]::new($png).GetThumbnailImage($size, $size, $null, [System.IntPtr]::Zero)
        $ms  = [System.IO.MemoryStream]::new()
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $images += [PSCustomObject]@{ Size = $size; Data = $ms.ToArray() }
        $ms.Dispose()
        $bmp.Dispose()
    }

    $count = $images.Count
    # ICO header: IDRESERVED(2) + IDTYPE(2) + IDCOUNT(2)
    $writer.Write([uint16]0)    # reserved
    $writer.Write([uint16]1)    # type: 1 = ICO
    $writer.Write([uint16]$count)

    $offset = 6 + $count * 16

    foreach ($img in $images) {
        $sz = if ($img.Size -ge 256) { 0 } else { $img.Size }
        $writer.Write([byte]$sz)            # width
        $writer.Write([byte]$sz)            # height
        $writer.Write([byte]0)              # color count
        $writer.Write([byte]0)              # reserved
        $writer.Write([uint16]1)            # planes
        $writer.Write([uint16]32)           # bit count
        $writer.Write([uint32]$img.Data.Length)
        $writer.Write([uint32]$offset)
        $offset += $img.Data.Length
    }

    foreach ($img in $images) {
        $writer.Write($img.Data)
    }

    $writer.Close()
    $stream.Close()
}

Write-Host "Converting $PngPath -> $OutPath ..."
ConvertTo-Icon -png $PngPath -outPath $OutPath
Write-Host "Done! ICO saved to: $OutPath"
