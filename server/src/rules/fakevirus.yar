rule fakevirus {
    strings:
        $fakeVirusMD5 = "9957e984be501c6d035e35573918ae4a"
        $fakeVirusSHA1 = "165ebd1a3f10e4e86f0b9cab32da718cadfe86b4"
        $fakeVirusSHA256 = "3696fbd121f6b0a103a11e7da33b640c106d1384c109f732072bc159f2965788"

    condition:
        any of them
}