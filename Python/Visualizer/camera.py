import numpy as np


class Camera:
    @staticmethod
    def frustum(left, right, bottom, top, z_near, z_far):
        M = np.zeros((4, 4), dtype=np.float32)
        M[0, 0] = +2.0 * z_near / (right - left)
        M[1, 1] = +2.0 * z_near / (top - bottom)
        M[2, 2] = -(z_far + z_near) / (z_far - z_near)
        M[0, 2] = (right + left) / (right - left)
        M[2, 1] = (top + bottom) / (top - bottom)
        M[2, 3] = -2.0 * z_near * z_far / (z_far - z_near)
        M[3, 2] = -1.0
        return M

    @staticmethod
    def perspective(fov_y, aspect, z_near, z_far):
        h = np.tan(0.5 * np.radians(fov_y)) * z_near
        w = h * aspect
        return Camera.frustum(-w, w, -h, h, z_near, z_far)

    @staticmethod
    def translate(x, y, z):
        return np.array([[1, 0, 0, x], [0, 1, 0, y],
                         [0, 0, 1, z], [0, 0, 0, 1]], dtype=float)

    @staticmethod
    def x_rotate(theta):
        t = np.pi * theta / 180
        c, s = np.cos(t), np.sin(t)
        return np.array([[1, 0, 0, 0], [0, c, -s, 0],
                         [0, s, c, 0], [0, 0, 0, 1]], dtype=float)

    @staticmethod
    def y_rotate(theta):
        t = np.pi * theta / 180
        c, s = np.cos(t), np.sin(t)
        return np.array([[c, 0, s, 0], [0, 1, 0, 0],
                         [-s, 0, c, 0], [0, 0, 0, 1]], dtype=float)
