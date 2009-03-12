// group, row, type, x, y, w, h, text style
static const TInt KRawLayoutData[][6] = {
	// ContactsList 
	LAYOUTGROUP( LG_contacts_list ),
	{ ERect, 0, 0, P - 6, P, ENoFont },  // * 0 list box
	{ ERect, P-6, 0,   6, P, ENoFont },  // * 1 scroll bar
	{ ERect, 0, 0, P - 0, 32, ENoFont }, // * 2 list item height		
	MARGINS4(2,1,1,1),                        // * 3 list item content size
	{ ERect, 0, 0, P - 6, 22, ENoFont }, // * find pane height

	// ContactsList 
	LAYOUTGROUP( LG_contacts_list_item_content ),
		
	{ EText, 0, 0,  P - 0, 15, ELogicFontPrimarySmall }, // * 0 Non-Jaiku Contact Name
	{ EText, 0, 15, P - 0, 15, ELogicFontSecondary }, // * 1 Non-Jaiku Contact extra details
	{ EText, 34, 0, P - 34, 15, ELogicFontPrimarySmall }, // * 2 Jaiku Contact Name  // FIXME, no way to specify half of parent height
	{ EText, 34, 15, P - 34, 15, ELogicFontSecondary },		// * 3 Jaiku Contact Status	
	{ EIcon, 0, 1, 30, 28, ENoFont },				// * 4 Jaiku Contact Image
	{ EIcon, P - 15, 0, 15, 15, ENoFont },			// * 5 Jaiku status icon
	{ EIcon, P - 12, P - 9, 10, 8, ENoFont },			// * 6 unread icon
	{ EIcon, P - 15, 0, 15, 15, ENoFont },  		// * 6 Selection marker
		
	// WelcomeLayout
	LAYOUTGROUP( LG_welcome_softkeys ), 
	// These are automatically positioned, x,y are used as margins and w,h as size
	{ EText, 4, 0, 80, 20, ETitleFont }, 	//   softkeys left 
	{ EText, 4, 0, 80, 20, ETitleFont },	//   softkeys right
		

	LAYOUTGROUP( LG_welcome_page ), 
	{ EText, 22, 144, P - 44, 54, EDenseFont }, // body text
	{ EIcon, 34, 20,  P - 68, 122, ENoFont },   // welcome logo  
	{ EIcon, 10, 126, P - 20, 15, ENoFont },   // welcome to jaiku text


	LAYOUTGROUP( LG_welcome_selection_page ), 
	{ EIcon,      0,     0,      P,     44, ENoFont }, // header bg		
	{ EIcon,     10,    14, P - 40,     28, ENoFont },   // header text,
	// x=7 (instead of 10) for some reason, approx 3 pixels are generated to left hand-side of header text 
	// y=14 manually decided (44 - 10 - (21 half + one quarter), but 2 pixels more are needed
	// h=28, half to text (14) and quarters (7) to above and below 
	
	{ EIcon, P - 38,     4,     32,     38, ENoFont },   //  header logo
	{ EText,     10,    51, P - 20,     60, EDenseFont },   //  selection body text
	// y = 51 instead of 54, for some reason approx 3 pixels are generated to top of label

	LAYOUTGROUP( LG_welcome_congratulations_page ), 
	{ EText, 22, 77, P - 44, 55, EDenseFont }, // body text
	{ EIcon, 72, 10,  32, 72, ENoFont },   // welcome logo  
	{ EIcon, 35, 65, P-70, 15, ENoFont },   // welcome to jaiku text

		
	// Welcome selection list
	LAYOUTGROUP( LG_welcome_selection_listbox ),
	{ ERect, 0, P - 25,   P, 0, ENoFont },      //   listbox
	{ ERect, 0, 0, P - 0, 32, ENoFont },        // single item
	{ EText, 10, 10, P - 20, P-15, EDenseFont },        // single item

		
	// Welcome selection list
	LAYOUTGROUP( LG_welcome_selection_listbox_content ),
	{ EText, 0, 0, P + 0, 16, EDenseFont }, 		// SelectionList 

		
	// Rich presence list 
	LAYOUTGROUP( LG_richpresence_list ),
	{ ERect, 0, 0, P - 6, P, ENoFont },     // * 0 List size
	{ ERect, 0, 0, P - 0, 34, ENoFont },    // * 1 List item full size
	MARGINS(2,2),                           // * 2 List item content size

	LAYOUTGROUP( LG_richpresence_list_item_content ),		
	{ EText,     20,    0,  P - 20,      15, ELogicFontPrimarySmall }, // * 1 main text
	{ EText, P - 20,    0,       0,      15, ELogicFontSecondary }, // * 2 main text tstamp
	{ EText,     20,   16,  P - 20,      15, ELogicFontSecondary  }, // * 3 secondary text
	{ EIcon,      0,    5,       20,       20, ENoFont  },    // * 4 icon

	LAYOUTGROUP( LG_idleview_connection_icon ),		
	{ EIcon,      1,   46,       8,       8, ENoFont }, // window size


	LAYOUTGROUP( LG_stream_list ),
	{ ERect, 0, 0, P - 6, P, ENoFont },     // * 0 List size
	{ ERect, 0, 0, P - 0, 17, ENoFont },    // * 1 List item full size
	MARGINS(1,1),                           // * 2 List item content size

	LAYOUTGROUP( LG_stream_list_item_content ),
	{ EText, 30,     0,  P-30,  P, ELogicFontPrimarySmall },     // * 0 Text
	{ EIcon, 18,     3,    10, 10, ENoFont  },             // * 1 jaicon
	{ EIcon, 0,      0,    16, 16, ENoFont  },             // * 1 buddy icon
	{ EIcon, P - 20, 3,    10, 10, ENoFont },     // * 2 Comment icon
	{ EText, P - 10, 0,    10,   P, ELogicFontSecondary },     // * 2 Comment count

	LAYOUTGROUP( LG_feed_controls ),
	{ EIcon, 0,      0,    30, 28, ENoFont  },             // * buddy icon
	{ EIcon, 0,      0,    20, 20, ENoFont  },             // * jaicon
	{ EIcon, 0,      0,    15, 15, ENoFont },	       // * status icon
	{ EIcon, 0,      0,    176, 2, ENoFont },	       // * separator
	{ ERect, P-6,    6,      3, 3, ENoFont },	       // * unread temporary indicator
	{ ERect, 0,      0,    50, 50, ENoFont  },             // * thumb nail



	LAYOUTGROUP( LG_feed_controls_margins ),
	{ EMarg, 1,      1,    1, 1, ENoFont  },             // * main margins
	{ EMarg, 2,      0,    2, 2, ENoFont  },             // * focusable buddy margins 
	{ EMarg, 1,      1,    1, 1, ENoFont  },             // * jaicon margins
	{ EMarg, 6,      2,    2, 2, ENoFont  },             // * bubble content margins
	{ EMarg, 1,      1,    1, 1, ENoFont  },             // * button out margins
	{ EMarg, 1,      1,    1, 1, ENoFont  },             // * button in margins
	{ EMarg, 0,      0,    2, 1, ENoFont  },             // * separator margins
	{ EMarg, 2,      2,    2, 2, ENoFont  },             // * author header
	{ EMarg, 2,      2,    2, 2, ENoFont  },             // * author header buddy
	{ EMarg, 1,      0,    0, 0, ENoFont  },             // * comment indicator
	{ EMarg, 3,      3,    3, 3, ENoFont  },             // * focusable buddy margins 
	{ EMarg, 2,      2,    2, 2, ENoFont  },             // * thumb nail margins


	LAYOUTGROUP( LG_feed_bubble ),
	{ ERect, 0,      0,   10,   5, ENoFont  },             // * bubble corner u1,u3,d1,d3
	{ ERect, 0,      0,   10, 200, ENoFont  },             // * bubble left side, m1
	{ ERect, 0,      0,   10, 200, ENoFont  },             // * bubble right side,
	{ ERect, 0,      0,  200,   5, ENoFont  },             // * bubble top side, u2
	{ ERect, 0,      0,  200,   5, ENoFont  },             // * bubble bottom side, d2

	LAYOUTGROUP( LG_feed_button ),
	{ ERect, 0,      0,   10,   25, ENoFont  },             // * button u1
	{ ERect, 0,      0,   10,   25, ENoFont  },             // * button u2
	{ ERect, 0,      0,   10,   25, ENoFont  },             // * button u3
	{ ERect, 0,      0,   10,    5, ENoFont  },             // * button d1
	{ ERect, 0,      0,   10,    5, ENoFont  },             // * button d2
	{ ERect, 0,      0,   10,    5, ENoFont  },             // * button d3	

	LAYOUTGROUP( LG_presence_list ),	
	{ EMarg,      0,    0,        2,       2, ENoFont  },    // item margins
	{ EIcon,      0,    0,       20,      20, ENoFont  },    // icon
	{ EMarg,      4,    4,        4,       4, ENoFont  },    // icon margins
	{ EIcon, 0,      0,    15, 15, ENoFont },	         // * status icon 

	LAYOUTGROUP( LG_progressbar ),	
	{ ERect,      0,    0,        0,       12, ENoFont  },    // progress bar


	LAYOUTGROUP( LG_mediaview ),
	{ ERect,      0,       0,        P,     P-45, ENoFont  },    // LI_mediaview__upper
	{ ERect,      0,     P-45,        P,       45, ENoFont  },    // LI_mediaview__lower
	{ ERect,     P-10,    15,       10,       30, ENoFont  },    // LI_mediapost__indicators

	LAYOUTGROUP( LG_mediaviewupper ),
	{ ERect,      0 ,     0,     P,      P, ENoFont  },    // LI_mediaviewupper__picture
	{ ERect,      0,      0,     P,      P, ENoFont  },    // LI_mediaviewupper__videorectangle

	{ ERect,      0,   64,       10,       14, ENoFont  },    // LI_mediaviewupper__leftarrow
	{ ERect,   P-10,   64,       10,       14, ENoFont  },    // LI_mediaviewupper__rightarrow

	{ EText,   P-34, P-17,       34,       17, ENoFont  },    // LI_mediaviewupper__timebox
	{ ERect,  P-100,    0,      100,       14, ENoFont  },    // LI_mediaviewupper__countbox
	{ ERect,      0,    0,    P-100,       14, ENoFont  },    // LI_mediaviewupper__loadinglabel
	{ ERect,     20,   30,      136,       60, ENoFont  },    // LI_mediaviewupper__errorlabel
	{ ERect,     73, P-40,       35,       20, ENoFont  },    // LI_mediaviewupper__volumeindicator
	

	LAYOUTGROUP( LG_mediaviewlower ),
	{ ERect,      8,      16,     P-2*8,    P- 15, ENoFont  },    // LI_mediaviewlower__comment
	{ ERect,      8,       0,     P-2*8,       16, ENoFont  },    // LI_mediaviewlower__tags


	LAYOUTGROUP( LG_mediapost ),
	{ ERect,      0,      30,        P,       14, ENoFont  },    // LI_mediapost__selecttags


	LAYOUTGROUP( LG_autotags ),
	{ ERect,        0,     0,       118,       15, ENoFont  },    // LI_autotags__itemsize
	{ ERect,       12,     0,      P-12,        P, ENoFont  },    // LI_autotags__text
	{ ERect,        0,     0,        12,        P, ENoFont  },    // LI_autotags__icon
	{ EMarg,        2,     0,         0,        0, ENoFont  },    // LI_autotags__textmargins
	{ EMarg,        2,     1,         0,        0, ENoFont  },    // LI_autotags__iconmargins
	
	LAYOUTGROUP( LG_medialistbox ),
	{ ERect,        0,     0,         P,        20, ENoFont  },    // LI_medialistbox__itemsize
	{ EText,        0,     0,       P-50,        P, ELogicFontPrimarySmall  },    // LI_medialistbox__name
	{ EText,     P-50,     0,         50,        P, ELogicFontPrimarySmall  },    // LI_medialistbox__count
	{ EMarg,       15,     1,          1,        2, ENoFont  },    // LI_medialistbox__name_margins
	{ EMarg,        2,    10,          3,        2, ENoFont  },    // LI_medialistbox__count_margins
	

	

	LAST()
};	



static const TConstDiffRow legacyPortraitDiff[] = {
	LASTDIFF()
};


static const TConstDiffRow legacyLandscapeDiff[] = {
	{ { LG_welcome_page, LI_welcome_page__body_text },      
	  { EText, 22, 115, P - 44, 56, EDenseFont } },

	{ { LG_welcome_page, LI_welcome_page__jaiku_logo },     
	  { EIcon, 34, 10,  P - 68, 90, ENoFont } }, 

	{ { LG_welcome_page, LI_welcome_page__welcometojaiku }, 
	  { EIcon, 10, 90,  P - 20, 15, ENoFont } },

	// selection listbox
	
	{ { LG_welcome_selection_listbox, LI_welcome_selection_listbox__item},
	  { ERect, 0, 0, P - 0, 20, ENoFont } },      
	
	{ { LG_welcome_selection_listbox, LI_welcome_selection_listbox__item_text},
	  { EText, 10, 3, P - 20, P-6, EDenseFont } },
	
	
	// congrats page 
  	{ {LG_welcome_congratulations_page, LI_welcome_congratulations_page__body_text},
	  { EText, 5, 75, P - 10, 55, EDenseFont } },
	

	{ {LG_welcome_congratulations_page, LI_welcome_congratulations_page__logo },
	  { EIcon, 88, 5,  P - 172, 82, ENoFont } }, 

	{ {LG_welcome_congratulations_page, LI_welcome_congratulations_page__text },
	  { EIcon, 35, 60, P-70, 15, ENoFont } }, 

	// idle view icon

	{ { LG_idleview_connection_icon, LI_idleview_connection_icon__icon},
	  { EIcon,      60,   17,      12,      12, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     30,      28, ENoFont } },
 	{ { LG_feed_controls, LI_feed_controls__separator},
	  { EIcon, 0,      0,    208, 2, ENoFont } },	       // * separator

 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    50, 50, ENoFont } },	       // * thumbnail


 	{ { LG_mediaview, LI_mediaview__upper },
	  { ERect,      0,       0,        P,     P-30, ENoFont } },    // LI_mediaview__upper
 	{ { LG_mediaview, LI_mediaview__lower },
	  { ERect,      0,    P-30,        P,       30, ENoFont  } },    // LI_mediaview__lower
 	{ { LG_mediaview, LI_mediaview__indicators },
	  { ERect,     P-10,    15,       10,       30, ENoFont  } },    // LI_mediapost__indicators


 	{ { LG_mediaviewlower, LI_mediaviewlower__comment },
	  { ERect,      2,      15,     P-2*2,     P-15, ENoFont  } },    // LI_mediaviewlower__comment
 	{ { LG_mediaviewlower, LI_mediaviewlower__tags },
	  { ERect,      2,       0,     P-2*2,       15, ENoFont  } },    // LI_mediaviewlower__tags

	
	{ { LG_autotags, LI_autotags__itemsize },
	  { ERect,        0,     0,       118,       14, ENoFont  } },    // LI_autotags__itemsize
	
	
	LASTDIFF()
};


static const TConstDiffRow doublePortraitDiff[] = {
	{ { LG_contacts_list, LI_contacts_list__find_pane}, 
	  { ERect, 0, 0, P - 12, 44, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     60,      56, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    120, 120, ENoFont } },	       // * thumbnail

	LASTDIFF()
};

static const TConstDiffRow doubleLandscapeDiff[] = {
	{ { LG_idleview_connection_icon, LI_idleview_connection_icon__icon}, 
	  { EIcon,      120,  34,      24,      24, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     60,      56, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    120, 120, ENoFont } },	       // * thumbnail

	LASTDIFF()
};

static const TConstDiffRow qvgaPortraitDiff[] = {
	// contact list
	{ {LG_contacts_list, LI_contacts_list__item_full},
	  { ERect, 0, 0, P - 0, 40, ENoFont } },
	{ {LG_contacts_list, LI_contacts_list__find_pane},
	  { ERect, 0, 0, P - 8, 30, ENoFont } },

	// contact list content
	{ { LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_image },
	  { EIcon, 0, 0, 40, 36, ENoFont } },
	
	// idle view icon
	{ { LG_idleview_connection_icon, LI_idleview_connection_icon__icon},
	  { EIcon,      1,   62,       14,      14, ENoFont } },
	
 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     40,      36, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__separator},
	  { EIcon, 0,      0,   240 , 2, ENoFont } },	       // * separator

 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    120, 120, ENoFont } },	       // * thumbnail


 	{ { LG_feed_controls_margins, LI_feed_controls_margins__main},
	{ EMarg, 2,      2,    2, 2, ENoFont  } },             // * main margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__buddy},
	{ EMarg, 2,      0,    2, 2, ENoFont  } },             // * focusable buddy margins 
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__jaicon},
	{ EMarg, 2,      2,    2, 2, ENoFont  } },             // * jaicon margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__bubblecontent},
	{ EMarg, 12,      4,    4, 4, ENoFont  } },             // * bubble content margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__button_out},
	{ EMarg, 2,      2,    2, 2, ENoFont  } },             // * button out margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__button_in},
	{ EMarg, 2,      2,    2, 2, ENoFont  } },             // * button in margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__separator},
	{ EMarg, 0,      0,    4, 2, ENoFont  } },             // * separator margins
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__author_header},
	{ EMarg, 4,      4,    4, 4, ENoFont  } },             // * author header
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__author_header_buddy},
	{ EMarg, 4,      4,    4, 4, ENoFont  } },             // * author header buddy
 	{ { LG_feed_controls_margins, LI_feed_controls_margins__comment_indicator},
	{ EMarg, 1,      0,    0, 0, ENoFont  } },             // * comment indicator

	{ GROUP_OPERATION( LG_feed_bubble ),
	  DOUBLE_BASE() },	
	
	{ GROUP_OPERATION( LG_feed_button ),
	  DOUBLE_BASE() },	

 	{ { LG_mediaview, LI_mediaview__upper },
	  { ERect,      0,       0,        P,     P-45, ENoFont  } },  // LI_mediaview__upper
	{ { LG_mediaview, LI_mediaview__lower },
	  { ERect,      0,     P-45,        P,       45, ENoFont  } },    // LI_mediaview__lower
	
 	{ { LG_mediaviewlower, LI_mediaviewlower__comment },
	  { ERect,      8,      15,     P-2*8,     P-15 , ENoFont  } },    // LI_mediaviewlower__comment
 	{ { LG_mediaviewlower, LI_mediaviewlower__tags },
	  { ERect,      8,       0,     P-2*8,       15, ENoFont  } },    // LI_mediaviewlower__tags


	LASTDIFF()
};

static const TConstDiffRow qvgaLandscapeDiff[] = {
	// contact list
	{ {LG_contacts_list, LI_contacts_list__item_full},
	  { ERect, 0, 0, P - 0, 30, ENoFont } },
	
	{ {LG_contacts_list, LI_contacts_list__item_content_only},
	  MARGINS(2,0) },
	
	{ {LG_contacts_list, LI_contacts_list__find_pane},
	  { ERect, 0, 0, P - 8, 26, ENoFont } },

	{ GROUP_OPERATION( LG_contacts_list_item_content ),
	  QVGALANDSCAPE_X() },	

	{ GROUP_OPERATION( LG_richpresence_list ),
	  QVGALANDSCAPE_X() },	

	{ GROUP_OPERATION( LG_richpresence_list_item_content ),
	  QVGALANDSCAPE_X() },	

	  
	
	// idle view icon
	{ { LG_idleview_connection_icon, LI_idleview_connection_icon__icon},
	  { EIcon,      76,   21,      14,      14, ENoFont } }, 

 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     30,      28, ENoFont } },


 	{ { LG_feed_controls, LI_feed_controls__separator},
	  { EIcon, 0,      0,   320 , 2, ENoFont } },		    // * separator


 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    120, 120, ENoFont } },	       // * thumbnail

	{ GROUP_OPERATION( LG_feed_bubble ),
	  DOUBLE_BASE() },	
	
	{ GROUP_OPERATION( LG_feed_button ),
	  DOUBLE_BASE() },	

	LASTDIFF()
};



static const TConstDiffRow e90Diff[] = {
	// contact list


	{ {LG_contacts_list, LI_contacts_list__item_full},
 	    { ERect, 0, 0, P - 0, 40, ENoFont } },

	{ {LG_contacts_list, LI_contacts_list__item_content_only},
	  MARGINS4(2,1,1,1) }, 

		     	
		
  	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__nonjaiku_contact_name },
	  { EText, 0, 0,  P - 0, 20, ELogicFontPrimarySmall } }, // * 0 Non-Jaiku Contact Name

	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__nonjaiku_contact_extradetails },
	  { EText, 0, 20, P - 0, 20, ELogicFontSecondary } }, // * 1 Non-Jaiku Contact extra details
	
	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_name },
	  { EText, 42, 0, P - 42, 20, ELogicFontPrimarySmall } }, // * 2 Jaiku Contact Name  // FIXME, no way to specify half of parent height
	
	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_presenceline },
	  { EText, 42, 20, P - 42, 20, ELogicFontSecondary } },		// * 3 Jaiku Contact Status	

	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_image },
	  { EIcon, 0, 2, 40, 36, ENoFont } },
	  
	  { {LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_status_icon },
	  { EIcon, P - 20, 0, 20, 20, ENoFont } },			// * 5 Jaiku status icon

	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__jaiku_contact_unread_icon },
	  { EIcon, P - 24, P - 18, 20, 16, ENoFont } },			// * 6 unread icon

	{ {LG_contacts_list_item_content, LI_contacts_list_item_content__selection_marker },
	  { EIcon, P - 20, 0, 20, 20, ENoFont } },  		// * 6 Selection marker


	{ { LG_idleview_connection_icon, LI_idleview_connection_icon__icon}, 
	  { EIcon,      145,  34,      24,      24, ENoFont } },
	  
 	{ { LG_feed_controls, LI_feed_controls__buddy_icon},
	  { EIcon,      0,   0,     40,     36, ENoFont } },

 	{ { LG_feed_controls, LI_feed_controls__separator},
	  { EIcon, 0,      0,   800 , 2, ENoFont } },		    // * separator

 	{ { LG_feed_controls, LI_feed_controls__thumbnail},
	  { EIcon, 0,      0,    120, 120, ENoFont } },	       // * thumbnail


	LASTDIFF()
};
