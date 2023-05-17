original dumper by Ph4nton: https://github.com/Ph4nton/lol-offset-dump

parser library: https://github.com/d99kris/rapidcsv

I updated it for 64 bit, and made it faster when theres a lot of patterns to scan.

It automatically finds the offset for **non** OFFSET TYPE patterns. For OFFSET TYPE, you still **need to manually enter the offset** until i automate that too.

EXAMPLE: **ADDRESS, randomAddy, "F3 0F 59 35 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0"**    (no more number at the end)

The usage is ALMOST the same, see original post: https://www.unknowncheats.me/forum/league-of-legends/386218-lol-offset-dump.html
