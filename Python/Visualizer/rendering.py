import matplotlib.pyplot as plt


def set_font(name='Adobe Devanagari', size=14):
    font_mapping = {'family': name, 'weight': 'normal', 'size': size}
    plt.rc('font', **font_mapping)
