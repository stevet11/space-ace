
# Space-Ace

## Build Requirements

- cmake
- git
- g++
- libsqlite3-dev

## Building under linux

The `git ... --depth 1` is a sparse checkout.  It only pulls the latest source, it doesn't pull down the full git history.

```
git clone github:stevet11/space-ace.git --depth 1
cd space-ace
git clone https://github.com/stevet11/door.git door++ --depth 1
git clone https://github.com/SRombauts/SQLiteCpp.git --depth 1
git clone https://github.com/jbeder/yaml-cpp.git --depth 1
mkdir build
cd build
cmake ..
make
```

## Calling from a BBS

### Enigma 1/2

In the config/menus/*-doors.hjson file add these entries:

```
    {
        value: { command: "S" }
        action: [
            {
                acs: EC0
                action: @menu:doorAce
            }
            {
                /* acs: EC1 */
                action: @menu:doorAceU8
            }
        ]
    }
```

```
    doorAce: {
        desc: Space Ace
        module: abracadabra
        config: {
            name: Space Ace
            dropFileType: DOOR
            cmd: /home/enigma/bbs/doors/spaceace.sh
            args: [
                "{dropFilePath}"                    
            ]
            nodeMax: 0
            tooManyArt: DOORMANY
            io: stdio
        }            
    }
```

```
    doorAceU8: {
        desc: Space Ace
        module: abracadabra
        config: {
            name: Space Construct
            dropFileType: DOOR
            cmd: /home/enigma/bbs/doors/spaceace.sh
            args: [
                "{dropFilePath}"                    
            ]
            nodeMax: 0
            tooManyArt: DOORMANY
            io: stdio
            encoding: utf8
        }            
    }
```

spaceace.sh

```
#!/bin/bash
trap "" SIGINT

cd /home/enigma/bbs/doors/spaceace/
./space-ace -d $1 
```

While space-ace can detect CP437/unicode, under Enigma it can't.

### Talisman

In menus/doors.toml add an entry:

```
[[menuitem]]
command = "RUNDOOR"
hotkey = "S"
data = "doors/spaceace.sh"
```

In the doors directory, create spaceace.sh:

```
#!/bin/bash
trap "" SIGINT

cd doors/spaceace
./space-ace -d ../../temp/$1/door.sys
```

And space-ace would be in doors/spaceace directory.

Look for space-ace.yaml, space-ace.log, and space-data.db to be created on first run.