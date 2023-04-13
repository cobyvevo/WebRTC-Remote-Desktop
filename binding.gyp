{
	"targets":[
		{
			"target_name": "screengrabber",
			"sources": [
				"CModules/main.cpp"
			],

			"link_settings": {
				"libraries": [
					"-lopencv_world470"
				],
				"library_dirs": [
					"A:/Desktop/C++/opencv/opencv/build/x64/vc16/lib",
				]
			},

		    "include_dirs": [
		      "C:/Users/cshir/AppData/Local/node-gyp/Cache/19.4.0/include/node",
		      "A:/Desktop/C++/opencv/opencv/build/include",
		    ]
		}
	]
}