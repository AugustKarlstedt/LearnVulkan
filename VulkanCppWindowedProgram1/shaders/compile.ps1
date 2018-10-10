$items = Get-ChildItem -Path .\* -Include *.vert, *.frag

foreach ($item in $items)
{
    glslangValidator.exe -e main -V $item
}