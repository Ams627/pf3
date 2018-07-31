r = {
    "tech": {
        "serverutc": "123456789",
        "servercpu": "12345",
        "computerid": "{guid}"
    },
    "fares": {
        "ftec": {
            "version": "989",
            "ndfversion": "822"
        },
        "orig": "Poole",
        "dest": "Oxford",
        "result":
            [
                {
                    "rlc": "YNG",
                    // plusbus element present only when pb is requested and permitted. (it will be present
                    // if pb is permitted at either origin or destination). A separate request for PB fares will
                    // be available to avoid sending huge amounts of data with the main request
                    "plusbus": {
                        "o": "5883", "d": "1215", "po": "H132", "pd": "J809",
                        "fares": {
                            "b0": { "a": 210, "c": 105 },
                            "b1": { "a": 210, "c": 105 },
                            "b2": { "a": 210, "c": 105 },
                            "b3": { "a": 210, "c": 105 },
                            "sw0": { "a": 210, "c": 105 },   // season weekly for origin
                            "sw1": { "a": 210, "c": 105 },   // season weekly for destination
                            "sm0": { "a": 210, "c": 105 },   // season monthly for origin
                            "sm1": { "a": 210, "c": 105 },   // season monthly for destination
                            "sq0": { "a": 210, "c": 105 },   // season quarterly for origin
                            "sq1": { "a": 210, "c": 105 },   // season quarterly for destination
                            "sa0": { "a": 210, "c": 105 },   // season annual for origin
                            "sa1": { "a": 210, "c": 105 },   // season annual for destination
                        }
                    },

                    "flows" :
                        [
                            {
                                "o": "Q202", "d": "Q235", "route": "00000", "id":1234567, "discind":1,
                                "fares":
                                [
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" },
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" },
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" }
                                ]
                            },
                            {
                                "o": "Q202", "d": "Q235", "route": "00000", "id":1234567, discind: 1,
                                "fares":
                                [
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" },
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" },
                                     { "a": "270", "c": "135", "t": "7DF", "r": "BE" }
                                ]
                            }
                        ]
                }
            ]
    }, // fares
    "times": {
        "ocrs": "POO", "dcrs": "SOU",
        "journeys": [
           { "dep": 600, "arr": 720 }, // times in minutes from midnight
           { "dep": 639, "arr": 749 },
        ]
    }

}