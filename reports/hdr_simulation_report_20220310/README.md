># HDR image simulation experiments

# Contents

Notes:

* In the following figures, "Entropy gain" refers to the difference

        (H(H) + H(L)) - (H(H)+H(L')),

    where H(·) is the entropy, H and L are the simulated high and low images, and 

        L' = L - Pred(L,H)
        
    is the residual image obtained by predicting the L image using the high image as reference.
    
* Results are averaged for 173 images (listed at the bottom of the document).

## Plot: `TwoNumericAnalyzer-columns_sigma__entropy_gain_high_ref-line-groupby__family_label.pdf`

This plot shows the observed entropy gains for different values of the L/H factor and different values of the Gaussian noise standard deviation.
In this figure, no x or y shift is applied.

## Plot: `TwoNumericAnalyzer-columns_sigma__entropy_gain_high_ref-line-groupby__family_label.pdf`

This plots shows the observed entropy gains for different x and y shifts for standard deviations 0 (top), 1 (middle), and 2 (bottom).

The following images show the same results for a single value of the standard deviation:

- `sigma0_ScalarNumeric2DAnalyzer-columns_shift_x__shift_y__entropy_gain_high_ref-colormap.pdf`
- `sigma1_ScalarNumeric2DAnalyzer-columns_shift_x__shift_y__entropy_gain_high_ref-colormap.pdf`
- `sigma2_ScalarNumeric2DAnalyzer-columns_shift_x__shift_y__entropy_gain_high_ref-colormap.pdf`

In this plot, the default L/H factor 7/11 is employed.


## Image list

Results shown in the previous images are the average obtained for the H and L pairs obtained for the following 173 images 
(for each selection of the L/H factor, the Gaussian noise standard deviation and the x and y shift).

- `./boats2020/image_20200527-215649-751_0201-u8be-1x5120x5120.raw`
- `./boats2020/image_20200508-092839-319_0099-u8be-1x5120x5120.raw`
- `./boats2020/image_20200511-051227-745_0091-u8be-1x5120x5120.raw`
- `./boats2020/image_20200531-092712-529_0167-u8be-1x5120x5120.raw`
- `./boats2020/image_20200508-092838-434_0085-u8be-1x5120x5120.raw`
- `./boats2020/image_20200714-100238-761_0471-u8be-1x5120x5120.raw`
- `./boats2020/image_20200511-051229-136_0113-u8be-1x5120x5120.raw`
- `./boats2020/image_20200511-051228-124_0097-u8be-1x5120x5120.raw`
- `./boats2020/image_20200508-092840-331_0115-u8be-1x5120x5120.raw`
- `./boats2020/image_20200516-074719-603_0057-u8be-1x5120x5120.raw`
- `./boats2020/image_20200508-092839-825_0107-u8be-1x5120x5120.raw`
- `./boats2020/image_20200530-020548-527_0309-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-368_0195-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-093_0159-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-172_0271-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-003_0031-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-553_0103-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-234_0003-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-647_0057-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-312_0115-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-677_0279-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-713_0153-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-750_0027-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-992_0015-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-944_0125-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-691_0121-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-862_0187-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-598_0167-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-773_0059-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135337-206_0319-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-405_0069-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-256_0035-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-562_0293-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-264_0225-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-900_0061-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-197_0129-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-829_0139-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-152_0065-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-380_0211-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-680_0105-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-149_0239-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-520_0055-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-669_0089-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-195_0303-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-138_0223-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-725_0169-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-026_0063-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-483_0181-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-689_0295-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-851_0171-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-219_0161-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-574_0309-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-517_0229-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-877_0029-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-394_0053-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-424_0275-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-781_0249-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-438_0117-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-287_0257-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-910_0077-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-966_0157-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-267_0051-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-276_0241-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-015_0047-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-955_0141-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-624_0025-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-815_0297-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-115_0191-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-230_0177-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-919_0267-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-450_0133-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-770_0233-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-933_0109-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-655_0247-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-119_0017-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-461_0149-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-747_0201-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-045_0269-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-321_0305-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135337-079_0317-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-498_0023-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-345_0163-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-702_0137-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-361_0005-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-208_0145-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-576_0135-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-542_0087-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-740_0011-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-104_0175-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-784_0075-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-666_0263-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-762_0043-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-413_0259-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-298_0273-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-978_0173-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-447_0307-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-621_0199-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-632_0215-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-644_0231-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-000_0205-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-896_0235-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-301_0099-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-700_0311-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-922_0093-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-795_0091-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-736_0185-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-068_0301-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-989_0189-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-130_0033-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-540_0261-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-402_0243-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-587_0151-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-874_0203-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-290_0083-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-357_0179-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-081_0143-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-011_0221-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-174_0097-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-494_0197-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-806_0107-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-866_0013-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-930_0283-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-509_0039-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-253_0209-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-565_0119-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-160_0255-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-070_0127-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-048_0095-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-416_0085-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135327-472_0165-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-635_0041-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-037_0079-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-888_0045-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135331-391_0227-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135319-383_0037-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-759_0217-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-183_0287-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-613_0009-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-840_0155-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-818_0123-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135322-163_0081-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-371_0021-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-487_0007-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-059_0111-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-127_0207-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-658_0073-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135326-334_0147-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-436_0291-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135323-427_0101-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-953_0315-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-506_0213-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-793_0265-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-531_0071-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135318-245_0019-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135328-610_0183-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135329-242_0193-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135321-279_0067-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-310_0289-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135330-885_0219-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135336-827_0313-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-804_0281-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-908_0251-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135333-034_0253-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-057_0285-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-528_0245-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135332-023_0237-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135335-942_0299-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135334-551_0277-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135320-141_0049-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135325-323_0131-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135324-185_0113-u8be-1x5120x5120.raw`
- `./fields2020/image_20201008-135317-108_0001-u8be-1x5120x5120.raw`
